// wlvm.zig
const std = @import("std");
const Allocator = std.mem.Allocator;
const testing = std.testing;
const assert = std.debug.assert;

const Expr = @import("expr.zig").Expr;

// ======================
// Virtual Machine
// ======================

pub const VM = struct {
    allocator: Allocator,
    registers: [32]*Expr, // Register file
    pc: usize = 0, // Program counter
    intern_pool: StringInterner, // Symbol deduplication

    // Bytecode instructions
    pub const OpCode = union(enum) {
        // Data movement
        load: struct { reg: u8, expr: *Expr }, // Load immediate
        move: struct { dst: u8, src: u8 }, // Register copy

        // Pattern matching
        match_head: struct { reg: u8, head: *Expr }, // Check expression head
        match_eq: struct { reg: u8, expected: *Expr }, // Exact match
        match_seq: struct { reg: u8, len: u8 }, // Sequence length check

        // Control flow
        jump: i16, // Relative jump
        jump_if: struct { // Conditional jump
            cond: bool, // Jump if true/false
            offset: i16,
        },
    };

    pub fn init(allocator: Allocator) !VM {
        return VM{
            .allocator = allocator,
            .registers = undefined,
            .intern_pool = StringInterner.init(),
        };
    }

    pub fn deinit(self: *VM) void {
        self.intern_pool.deinit(self.allocator);
    }

    // Create expression (automatically interns symbols)
    pub fn createExpr(self: *VM, data: Expr.Data) !*Expr {
        var final_data = data;
        if (final_data == .symbol) {
            final_data.symbol = try self.intern_pool.intern(self.allocator, final_data.symbol);
        }

        const expr = try self.allocator.create(Expr);
        expr.* = .{ .data = final_data };
        return expr;
    }

    // Create normal expression f[x,y]
    pub fn mkNormal(self: *VM, head: *Expr, args: []*Expr) !*Expr {
        const args_copy = try self.allocator.dupe(*Expr, args);
        return self.createExpr(.{ .normal = .{ .head = head, .args = args_copy } });
    }

    fn destroyExpr(self: *VM, expr: *Expr) void {
        switch (expr.data) {
            .string => |s| self.allocator.free(s),
            .normal => |n| {
                self.release(n.head);
                for (n.args) |arg| self.release(arg);
                self.allocator.free(n.args);
            },
            else => {}, // Integers/symbols need no cleanup
        }
        self.allocator.destroy(expr);
    }

    // ======================
    // Pattern Compiler
    // ======================

    pub fn compilePattern(self: *VM, pattern: *Expr) ![]OpCode {
        var bytecode = std.ArrayList(OpCode).init(self.allocator);
        errdefer bytecode.deinit();

        switch (pattern.data) {
            .integer, .string, .symbol => {
                // Literal match
                try bytecode.append(.{ .match_eq = .{ .reg = 0, .expected = pattern } });
            },
            .normal => |n| {
                // Match head first
                try bytecode.append(.{ .match_head = .{ .reg = 0, .head = n.head } });

                // Match each argument
                try bytecode.append(.{ .match_seq = .{ .reg = 0, .len = @intCast(u8, n.args.len) } });

                for (n.args, 0..) |arg, i| {
                    // Move to next register window
                    try bytecode.append(.{ .move = .{ .dst = @intCast(u8, i) + 1, .src = 0 } });

                    // Compile sub-pattern
                    const sub_code = try self.compilePattern(arg);
                    defer self.allocator.free(sub_code);
                    try bytecode.appendSlice(sub_code);
                }
            },
        }

        return bytecode.toOwnedSlice();
    }

    // ======================
    // VM Execution
    // ======================

    pub fn execute(self: *VM, bytecode: []OpCode) !bool {
        self.pc = 0;
        while (self.pc < bytecode.len) {
            const op = bytecode[self.pc];
            self.pc += 1;

            switch (op) {
                .load => |l| self.registers[l.reg] = l.expr,
                .move => |m| self.registers[m.dst] = self.registers[m.src],

                .match_head => |m| {
                    const expr = self.registers[m.reg];
                    const expected_head = m.head.data.symbol;

                    const actual_head = switch (expr.data) {
                        .symbol => expr.data.symbol,
                        .normal => expr.data.normal.head.data.symbol,
                        else => return false,
                    };

                    if (!std.mem.eql(u8, actual_head, expected_head)) {
                        return false;
                    }
                },

                .match_eq => |m| {
                    if (self.registers[m.reg] != m.expected) {
                        return false;
                    }
                },

                .match_seq => |m| {
                    const expr = self.registers[m.reg];
                    if (expr.data != .normal or expr.data.normal.args.len != m.len) {
                        return false;
                    }
                },

                .jump => |offset| {
                    self.pc = @intCast(usize, @intCast(isize, self.pc) + offset);
                },

                .jump_if => |j| {
                    const cond = switch (self.registers[0].data) {
                        .integer => |i| i != 0,
                        .symbol => |s| std.mem.eql(u8, s, "True"),
                        else => false,
                    };
                    if (cond == j.cond) {
                        self.pc = @intCast(usize, @intCast(isize, self.pc) + j.offset);
                    }
                },
            }
        }
        return true;
    }

    // High-level MatchQ interface
    pub fn matchQ(self: *VM, expr: *Expr, pattern: *Expr) !bool {
        // Load expression into register 0
        self.registers[0] = expr;

        // Compile pattern to bytecode
        const bytecode = try self.compilePattern(pattern);
        defer self.allocator.free(bytecode);

        // Execute matcher
        return self.execute(bytecode);
    }
};

// ======================
// String Interning
// ======================

const StringInterner = struct {
    map: std.StringHashMapUnmanaged(void),

    pub fn init() StringInterner {
        return .{
            .map = .empty,
        };
    }

    pub fn deinit(self: *StringInterner, allocator: Allocator) void {
        var it = self.map.iterator();
        while (it.next()) |entry| {
            allocator.free(entry.key_ptr.*);
        }
        self.map.deinit(allocator);
    }

    pub fn intern(self: *StringInterner, allocator: Allocator, str: []const u8) ![]const u8 {
        const res = try self.map.getOrPut(str);
        if (!res.found_existing) {
            const owned = try allocator.dupe(u8, str);
            res.key_ptr.* = owned;
        }
        return res.key_ptr.*;
    }
};

// ======================
// Test Cases
// ======================

test "basic pattern matching" {
    var vm = try VM.init(testing.allocator);
    defer vm.deinit();

    // Create symbols
    const f = try vm.createExpr(.{ .symbol = "f" });
    const x = try vm.createExpr(.{ .symbol = "x" });
    const two = try vm.createExpr(.{ .integer = 2 });

    // Build f[x, 2]
    const expr = try vm.mkNormal(f, &.{ x, two });

    // Test pattern f[_, _]
    const pat_head = try vm.createExpr(.{ .symbol = "f" });
    const pat = try vm.mkNormal(pat_head, &.{ try vm.createExpr(.{ .symbol = "_" }), try vm.createExpr(.{ .symbol = "_" }) });

    try testing.expect(try vm.matchQ(expr, pat));
}

test "bytecode execution" {
    var vm = try VM.init(testing.allocator);
    defer vm.deinit();

    // Create test expression
    const expr = try vm.mkNormal(try vm.createExpr(.{ .symbol = "Plus" }), &.{ try vm.createExpr(.{ .integer = 1 }), try vm.createExpr(.{ .integer = 2 }) });

    // Load into register 0
    vm.registers[0] = expr;

    // Compile pattern Plus[_,_]
    const bytecode = &[_]VM.OpCode{
        .{ .match_head = .{ .reg = 0, .head = try vm.createExpr(.{ .symbol = "Plus" }) } },
        .{ .match_seq = .{ .reg = 0, .len = 2 } },
    };

    try testing.expect(try vm.execute(bytecode));
}
