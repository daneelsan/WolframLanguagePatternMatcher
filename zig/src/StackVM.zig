const std = @import("std");
const wlr = @import("WolframLanguageRuntime.zig");
const Allocator = std.mem.Allocator;

const Expr = wlr.Expr;

pub const VM = struct {
    allocator: Allocator,
    stack: std.ArrayList(Frame),
    current_frame: ?*Frame,
    pattern: []const Opcode,
    matched: bool = false,

    const Frame = struct {
        expr: wlr.Expr,
        pc: usize,
        slots: []?wlr.Expr,
        backtrack_pc: usize,
        seq_state: SeqState = .{},
    };

    const SeqState = struct {
        start: usize = 0,
        current_len: usize = 0,
        min: usize = 0,
        max: usize = std.math.maxInt(usize),
    };

    pub fn init(allocator: Allocator, pattern: []const Opcode) VM {
        return .{
            .allocator = allocator,
            .stack = std.ArrayList(Frame).init(allocator),
            .current_frame = null,
            .pattern = pattern,
        };
    }

    pub fn deinit(self: *VM) void {
        for (self.stack.items) |*frame| {
            self.allocator.free(frame.slots);
        }
        self.stack.deinit();
    }

    pub const Opcode = union(enum) {
        push_scope: usize,
        pop_scope,
        match_head: wlr.Expr, // Symbol
        match_type: wlr.Expr.ExprTag,
        match_slot: usize,
        match_all: struct { min: usize, max: usize },
        jump_if_fail: usize,
        commit,
    };

    // Core execution
    pub fn execute(self: *VM, expr: wlr.Expr) !bool {
        try self.pushFrame(expr, 0);

        while (!self.matched and self.current_frame != null) {
            const frame = self.current_frame.?;
            if (frame.pc >= self.pattern.len) {
                try self.handleSuccess();
                continue;
            }

            const op = self.pattern[frame.pc];
            frame.pc += 1;

            switch (op) {
                .push_scope => |num_slots| {
                    try self.pushFrame(frame.expr, num_slots);
                    self.current_frame.?.backtrack_pc = frame.pc;
                },
                .pop_scope => _ = self.popFrame(),
                .match_head => |expected| {
                    const head = try frame.expr.part(0); // Head is at position 0
                    if (!head.sameQ(expected)) {
                        try self.backtrack();
                    }
                },
                .match_type => |expected_type| {
                    // TODO:
                    if (frame.expr != expected_type) {
                        try self.backtrack();
                    }
                },
                .match_slot => |slot_idx| {
                    if (slot_idx >= frame.slots.len) return error.InvalidSlot;
                    frame.slots[slot_idx] = frame.expr;
                },
                .match_all => |range| {
                    try self.handleSequenceMatch(frame, range);
                },
                .jump_if_fail => |offset| {
                    frame.backtrack_pc = frame.pc + offset;
                },
                .commit => self.matched = true,
            }
        }
        return self.matched;
    }

    fn handleSequenceMatch(self: *VM, frame: *Frame, range: struct { min: usize, max: usize }) !void {
        const expr_len = try frame.expr.length();

        // Initialize sequence state on first encounter
        if (frame.seq_state.current_len == 0) {
            frame.seq_state = .{
                .min = range.min,
                .max = range.max,
                .start = 0,
            };
        }

        const remaining = expr_len - frame.seq_state.start;
        if (frame.seq_state.current_len < range.min and remaining == 0) {
            try self.backtrack();
            return;
        }

        // Bind current segment
        if (frame.seq_state.current_len < range.max and remaining > 0) {
            const end = frame.seq_state.start + 1;
            const slice = try self.exprSequence(self.allocator, self.frame.expr, frame.seq_state.start, end);
            frame.slots[0] = slice; // Store in first slot
            frame.seq_state.start = end;
            frame.seq_state.current_len += 1;
            frame.pc -= 1; // Re-execute for greedy matching
        }
    }

    fn pushFrame(self: *VM, expr: wlr.Expr, num_slots: usize) !void {
        const frame = Frame{
            .expr = expr,
            .pc = 0,
            .slots = try self.allocator.alloc(?wlr.Expr, num_slots),
            .backtrack_pc = 0,
        };
        try self.stack.append(frame);
        self.current_frame = &self.stack.items[self.stack.items.len - 1];
    }

    fn popFrame(self: *VM) void {
        _ = self.stack.pop();
        self.current_frame = if (self.stack.items.len > 0)
            &self.stack.items[self.stack.items.len - 1]
        else
            null;
    }

    fn backtrack(self: *VM) !void {
        while (self.current_frame) |frame| {
            if (frame.pc < frame.backtrack_pc) {
                frame.pc = frame.backtrack_pc;
                frame.seq_state = .{}; // Reset sequence state
                return;
            }
            _ = self.popFrame();
        }
        return error.MatchFailed;
    }

    fn handleSuccess(self: *VM) !void {
        self.matched = true;
        _ = self.popFrame();
    }

    pub fn exprSequence(allocator: Allocator, expr: wlr.Expr, start: usize, end: usize) !wlr.Expr {
        return switch (expr) {
            .Normal => blk: {
                const slice_expr = try wlr.Expr.symbol("System`Sequence");
                var parts = try std.ArrayList(wlr.Expr).initCapacity(allocator, end - start);
                defer parts.deinit();

                for (start..end) |i| {
                    try parts.append(try expr.part(@intCast(i + 1))); // Parts are 1-indexed
                }
                break :blk try wlr.Expr.construct(slice_expr, parts.items);
            },
            else => error.UnexpectedType,
        };
    }
};

// Example pattern compilation
pub fn compilePattern(allocator: Allocator, pattern_head: wlr.Expr) ![]const VM.Opcode {
    var pattern = std.ArrayList(VM.Opcode).init(allocator);

    // Example: Compile f[___, y_]
    try pattern.append(.{ .push_scope = 2 });
    try pattern.append(.{ .match_head = pattern_head });
    try pattern.append(.{ .match_all = .{ .min = 0, .max = std.math.maxInt(usize) } });
    try pattern.append(.{ .match_slot = 1 }); // y_
    try pattern.append(.commit);

    return pattern.toOwnedSlice();
}
