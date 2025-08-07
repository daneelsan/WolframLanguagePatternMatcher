const Expr = @import("Expr.zig");
const EmbedSupport = @import("EmbedSupport.zig");
const wrtl = @import("WolframRTL.zig");

const std = @import("std");

pub const MExpr = struct {
    allocator: std.mem.Allocator,
    expr: Expr,

    pub fn init(allocator: std.mem.Allocator, expr: Expr) !*MExpr {
        const self = try allocator.create(MExpr);
        self.* = .{
            .allocator = allocator,
            .expr = expr,
        };
        return self;
    }

    pub fn deinit(self: *MExpr) void {
        const allocator = self.allocator;
        allocator.destroy(self);
    }

    pub fn length(self: *MExpr) usize {
        return self.expr.length();
    }

    pub fn print(self: *MExpr) void {
        self.expr.print();
    }

    // EmbedSupport

    // pub fn embeddable(self: *MExpr) EmbedSupport.Embeddable {
    //     return .{
    //         .ptr = self,
    //         .vtable = &.{
    //             .alloc = alloc,
    //             .resize = resize,
    //             .remap = remap,
    //             .free = free,
    //         },
    //     };
    // }

    pub const embed_name = "MExpr";

    pub fn initializeEmbedMethods() void {
        _ = wrtl.AddCompilerClassMethod_Export(embed_name, "length", @ptrCast(&length_export));
        _ = wrtl.AddCompilerClassMethod_Export(embed_name, "toString", @ptrCast(&toString_export));

        _ = wrtl.SetClassRawMethod(embed_name, "releaseInstance", @ptrCast(&deinit));
    }

    export fn length_export(arg: wrtl.OpaqueExpr) wrtl.OpaqueExpr {
        const args: Expr = Expr.init(arg, true);
        if (args.length() != 1) {
            return Expr.errorException("Incorrect number of arguments for MExpr method \"length\".").returnValue();
        }

        const self = args.head();
        if (EmbedSupport.unembedObject(MExpr, self)) |mexpr| {
            const len = mexpr.length();
            return Expr.fromInt(len).returnValue();
        }
        return Expr.errorException("Could not extract MExpr for method \"length\".").returnValue();
    }

    export fn toString_export(arg: wrtl.OpaqueExpr) wrtl.OpaqueExpr {
        const args: Expr = Expr.init(arg, true);
        if (args.length() != 1) {
            return Expr.errorException("Incorrect number of arguments for MExpr method \"toString\".").returnValue();
        }

        const self = args.head();
        if (EmbedSupport.unembedObject(MExpr, self)) |mexpr| {
            mexpr.print();
            return Expr.inertExpression("Null").returnValue();
        }
        return Expr.errorException("Could not extract MExpr for method \"length\".").returnValue();
    }
};

pub const MExprFactory = struct {
    allocator: std.mem.Allocator,

    pub const embed_name = "MExprFactory";
};
