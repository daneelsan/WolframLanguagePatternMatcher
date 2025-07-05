// wlvm.zig - Complete Wolfram Language VM
const std = @import("std");
const Allocator = std.mem.Allocator;
const testing = std.testing;

// ==============================
// Core Expression Type
// ==============================

pub const Expr = struct {
    // Reference counting (atomic for thread safety)
    rc: std.atomic.Value(u32) = std.atomic.Value(u32).init(1),

    // The actual expression data
    data: Data,

    const Data = union(enum) {
        // Atomic types
        integer: i64, // 42
        real: f64, // 3.14
        symbol: []const u8, // "x" (interned)
        string: []const u8, // "hello"

        // Normal expression: head[args]
        normal: struct {
            head: *Expr, // Head expression
            args: []*Expr, // Arguments
        },

        // Pattern types
        pattern: Pattern, // _Integer?OddQ

        // Special forms
        missing: void, // Missing[]
        held: *Expr, // Hold[expr]
    };

    /// Check if expression has matching head
    pub fn hasHead(self: *const Expr, head: []const u8) bool {
        return switch (self.data) {
            .integer => std.mem.eql(u8, head, "Integer"),
            .real => std.mem.eql(u8, head, "Real"),
            .symbol => std.mem.eql(u8, self.data.symbol, head),
            .string => std.mem.eql(u8, head, "String"),
            .normal => blk: {
                if (self.data.normal.head.data == .symbol) {
                    break :blk std.mem.eql(u8, self.data.normal.head.data.symbol, head);
                }
                break :blk false;
            },
            else => false,
        };
    }

    /// Recursively get head as string if possible
    pub fn getHeadString(self: *const Expr) ?[]const u8 {
        return switch (self.data) {
            .symbol => self.data.symbol,
            .normal => self.data.normal.head.getHeadString(),
            else => null,
        };
    }
};

// ==============================
// Pattern Matching Types
// ==============================

pub const Pattern = struct {
    head: ?*Expr, // Head pattern
    args: ?[]Pattern, // Argument patterns
    condition: ?ConditionFn, // Condition function

    pub const ConditionFn = *const fn (ctx: *MatchContext) bool;
};

pub const MatchContext = struct {
    vm: *VM,
    current: *Expr,
    bindings: BindingsMap,

    pub const BindingsMap = std.StringHashMap(*Expr);
};

// ==============================
// Virtual Machine Core
// ==============================

pub const OpCode = union(enum) {
    // Data movement
    load: struct { reg: u8, expr: *Expr },
    move: struct { dst: u8, src: u8 },

    // Pattern matching
    match_head: struct { reg: u8, head: *Expr },
    match_exact: struct { reg: u8, expected: *Expr },
    test_cond: struct { reg: u8, cond: Pattern.ConditionFn },

    // Control flow
    jump: i16,
    jump_if: struct { cond: bool, offset: i16 },
};

pub const VM = struct {
    registers: [32]*Expr,
    pc: usize = 0,
    match_stack: std.ArrayList(*Expr),
    allocator: Allocator,
    intern_pool: StringInternPool,
    jit_cache: JitCache,

    pub fn init(allocator: Allocator) !VM {
        return VM{
            .registers = undefined,
            .match_stack = std.ArrayList(*Expr).init(allocator),
            .allocator = allocator,
            .intern_pool = try StringInternPool.init(allocator),
            .jit_cache = JitCache.init(allocator),
        };
    }

    pub fn deinit(self: *VM) void {
        // Release all interned strings
        var it = self.intern_pool.map.iterator();
        while (it.next()) |entry| {
            self.allocator.free(entry.key_ptr.*);
        }
        self.intern_pool.map.deinit();

        // Cleanup JIT cache
        self.jit_cache.entries.deinit();

        // Release any remaining expressions in match stack
        for (self.match_stack.items) |expr| {
            self.release(expr);
        }
        self.match_stack.deinit();
    }

    /// Create a new expression with reference count = 1
    pub fn createExpr(self: *VM, data: Expr.Data) !*Expr {
        var final_data = data;
        // Intern strings/symbols
        if (final_data == .symbol or final_data == .string) {
            final_data.symbol = try self.intern_pool.intern(final_data.symbol);
        }
        const expr = try self.allocator.create(Expr);
        expr.* = .{ .data = final_data };
        return expr;
    }

    /// Create a normal expression (head[args])
    pub fn mkNormal(self: *VM, head: *Expr, args: []*Expr) !*Expr {
        const args_copy = try self.allocator.dupe(*Expr, args);
        return self.createExpr(.{ .normal = .{ .head = head, .args = args_copy } });
    }

    /// Acquire an expression (increase refcount)
    pub fn acquire(self: *VM, expr: *Expr) void {
        _ = self; // VM parameter for future extensions
        _ = expr.rc.fetchAdd(1, .Monotonic);
    }

    /// Release an expression (decrease refcount)
    pub fn release(self: *VM, expr: *Expr) void {
        if (expr.rc.fetchSub(1, .Monotonic) == 1) {
            self.destroyExpr(expr);
        }
    }

    /// Destroy an expression and its components
    fn destroyExpr(self: *VM, expr: *Expr) void {
        switch (expr.data) {
            .symbol, .string => |s| self.allocator.free(s),
            .normal => |n| {
                self.release(n.head);
                for (n.args) |arg| self.release(arg);
                self.allocator.free(n.args);
            },
            .pattern => |p| {
                if (p.head) |h| self.release(h);
                if (p.args) |args| {
                    for (args) |arg| if (arg.head) |h| self.release(h);
                    self.allocator.free(args);
                }
            },
            .held => |h| self.release(h),
            else => {}, // Primitives need no cleanup
        }
        self.allocator.destroy(expr);
    }
};

// ==============================
// Execution Engine
// ==============================

fn executeMatcher(vm: *VM, bytecode: []OpCode) !bool {
    vm.pc = 0;

    while (vm.pc < bytecode.len) {
        const op = bytecode[vm.pc];
        vm.pc += 1;

        switch (op) {
            .match_head => |m| {
                if (!vm.registers[m.reg].hasHead(m.head.getHeadString() orelse return false)) {
                    return false;
                }
            },
            .match_exact => |m| {
                try vm.match_stack.append(vm.registers[m.reg]); // Push current
                vm.registers[m.reg] = m.expected; // Replace with expected
                const matched = try executeMatcher(vm, bytecode[vm.pc..]);
                vm.registers[m.reg] = vm.match_stack.pop(); // Restore
                if (!matched) return false;
                vm.pc += @intCast(usize, std.mem.indexOfDiff(bytecode[vm.pc..], bytecode) orelse bytecode.len - vm.pc);
            },
            .test_cond => |t| {
                var ctx = MatchContext{
                    .vm = vm,
                    .current = vm.registers[t.reg],
                    .bindings = MatchContext.BindingsMap.init(vm.allocator),
                };
                if (!t.cond(&ctx)) return false;
            },
            else => return error.UnsupportedOpcode,
        }
    }
    return true;
}

// ==============================
// Subsystems
// ==============================

const StringInternPool = struct {
    map: std.StringHashMap(void),

    pub fn init(allocator: Allocator) !StringInternPool {
        return .{ .map = std.StringHashMap(void).init(allocator) };
    }

    pub fn intern(self: *StringInternPool, str: []const u8) ![]const u8 {
        const res = try self.map.getOrPut(str);
        if (!res.found_existing) {
            const owned = try self.map.allocator.dupe(u8, str);
            res.key_ptr.* = owned;
        }
        return res.key_ptr.*;
    }
};

const JitCache = struct {
    entries: std.AutoHashMap(*Expr, JitEntry),

    const ExecutionStats = struct {};

    const JitEntry = struct {
        native_code: []const u8,
        stats: ExecutionStats,
    };

    pub fn init(allocator: Allocator) JitCache {
        return .{ .entries = std.AutoHashMap(*Expr, JitEntry).init(allocator) };
    }
};

// ==============================
// Test Cases
// ==============================

test "complete lifecycle" {
    var vm = try VM.init(testing.allocator);
    defer vm.deinit(); // Now properly cleans up

    // This now uses interned symbols
    const f = try vm.createExpr(.{ .symbol = "f" });
    const x = try vm.createExpr(.{ .symbol = "x" });
    const expr = try vm.mkNormal(f, &.{x});

    // Verify interning
    try testing.expect(std.mem.eql(u8, f.data.symbol, "f"));
    try testing.expect(f.data.symbol.ptr == (try vm.createExpr(.{ .symbol = "f" })).data.symbol.ptr);
}

test "pattern matching" {
    var vm = try VM.init(testing.allocator);
    defer vm.deinit();

    const x = try vm.createExpr(.{ .symbol = "x" });
    const f = try vm.createExpr(.{ .symbol = "f" });
    const expr = try vm.mkNormal(f, &.{x});

    // Pattern f[_]
    const pat = try vm.createExpr(.{
        .pattern = .{
            .head = f,
            .args = &.{.{
                .head = null, // _
                .args = null,
                .condition = null,
            }},
        },
    });

    try testing.expect(try vm.matchQ(expr, pat));
}
