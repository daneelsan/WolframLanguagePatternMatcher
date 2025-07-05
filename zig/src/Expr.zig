pub const Expr = struct {
    refcount: RefCount = .init(),
    data: UncountedExpr = undefined,

    pub const RefCount = struct {
        count: std.atomic.Value(usize),
        dropFn: *const fn (*RefCount) void,

        pub fn init() RefCount {
            return .{
                .count = .init(1),
                .dropFn = noop,
            };
        }

        fn ref(rc: *RefCount) void {
            // no synchronization necessary; just updating a counter.
            _ = rc.count.fetchAdd(1, .monotonic);
        }

        fn unref(rc: *RefCount) void {
            // release ensures code before unref() happens-before the
            // count is decremented as dropFn could be called by then.
            if (rc.count.fetchSub(1, .release) == 1) {
                // seeing 1 in the counter means that other unref()s have happened,
                // but it doesn't mean that uses before each unref() are visible.
                // The load acquires the release-sequence created by previous unref()s
                // in order to ensure visibility of uses before dropping.
                _ = rc.count.load(.acquire);
                (rc.dropFn)(rc);
            }
        }

        fn noop(rc: *RefCount) void {
            _ = rc;
        }
    };

    pub const Type = struct {
        pub const Integer = struct {
            data: Data,

            pub const Data = i64;

            // TODO: SHAREDINTEGERS
            pub fn init(_: Allocator, data: Data) !Integer {
                return .{ .data = data };
            }

            pub fn deinit(allocator: Allocator) void {
                _ = allocator;
            }
        };

        pub const Real = struct {
            data: Data,

            pub const Data = f64;

            // TODO: SHAREDREALS
            pub fn init(_: Allocator, data: Data) !Real {
                return .{ .data = data };
            }

            pub fn deinit(allocator: Allocator) void {
                _ = allocator;
            }
        };

        pub const Normal = struct {
            // TODO: timestamp
            elems: []*Expr,

            pub fn init(_: Allocator, arg_count: usize) Normal {}

            pub fn deinit(allocator: Allocator) void {
                _ = allocator;
            }
        };
        pub const Symbol = struct {
            // TODO: timestamps

            pub fn free(allocator: Allocator) void {
                _ = allocator;
            }
        };

        // TODO: Support UTF-8
        pub const String = struct {
            data: [:0]const u8,

            pub fn free(allocator: Allocator) void {
                _ = allocator;
            }
        };
    };

    pub const ExprTag = enum {
        mint,
        // TODO: bigint
        mreal,
        // TODO: bigreal
        // TODO: rational
        // TODO: complex
        normal,
        symbol,
        string,
        // TODO: raw
    };

    pub const UncountedExpr = union(ExprTag) {
        mint: Type.Integer, // Machine integer
        mreal: Type.Real, // Machine real
        normal: Type.Normal, // f[x,y]
        symbol: Type.Symbol, // Interned string
        string: Type.String, // Non-interned
    };

    pub fn init(allocator: Allocator, uncounted_expr: UncountedExpr) Expr {}

    // pub fn refincr(self: *Expr) void {
    //     _ = self.rc.fetchAdd(1, .monotonic);
    // }

    // pub fn release(self: *Expr, vm: *VM) void {
    //     if (self.rc.fetchSub(1, .monotonic) == 1) {
    //         vm.destroyExpr(self);
    //     }
    // }
};

const std = @import("std");
const Allocator = std.mem.Allocator;
const testing = std.testing;
const assert = std.debug.assert;
