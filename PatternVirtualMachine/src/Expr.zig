/// Expr wraps a Wolfram expression pointer with manual reference counting.
/// - Creating an Expr with `init(..., count=false)` does NOT acquire a reference.
/// - Creating an Expr with `init(..., count=true)` DOES acquire a reference.
/// - `copy()` creates a new Expr and acquires a reference.
/// - You MUST call `deinit()` on every Expr you own to release its reference.
/// - Failing to call `deinit()` will leak memory; calling it twice will cause a double-free.
/// - Functions returning Expr (except `copy`) return a new reference (not acquired).
const Expr = @This();

const wrtl = @import("WolframRTL.zig");

const std = @import("std");

c_expr: wrtl.OpaqueExpr,

/// Create from an instance, if count is true then acquire
pub fn init(c_expr: wrtl.OpaqueExpr, count: bool) Expr {
    var self = Expr{ .c_expr = c_expr };
    if (count) {
        _ = self.acquire();
    }
    return self;
}

pub fn wrap(c_expr: wrtl.OpaqueExpr) Expr {
    return .{ .c_expr = c_expr };
}

pub fn unwrap(expr: Expr) wrtl.OpaqueExpr {
    return expr.c_expr;
}

/// Create from integer
pub fn fromInteger(x: i64) Expr {
    // Ensure pointer is aligned correctly
    const x_ptr = &x; // type: *const i64
    const src: [*c]const i64 = @ptrCast(@alignCast(@constCast(x_ptr)));
    const c_expr = wrtl.CreateIntegerExpr(src, 64, true);
    return Expr.init(c_expr, false);
}

/// Create from (c)string
pub fn fromCString(txt: []const u8) Expr {
    // NOTE: This assumes txt is null-terminated or we'd need to handle that
    const c_expr = wrtl.UTF8BytesAndLengthToStringExpression(txt.ptr, @intCast(txt.len), @intCast(txt.len));
    return Expr.init(c_expr, false);
}

/// Create from UTF8
pub fn fromUTF8(bytes: []const u8, len: usize) Expr {
    const c_expr = wrtl.UTF8BytesAndLengthToStringExpression(bytes.ptr, @intCast(bytes.len), len);
    return Expr.init(c_expr, false);
}

/// Copy constructor equivalent
pub fn copy(self: *const Expr) Expr {
    return Expr.init(self.c_expr, true);
}

/// Destructor equivalent
pub fn deinit(self: *Expr) void {
    _ = self.release();
}

pub fn refcount(self: *const Expr) usize {
    return @intCast(wrtl.ExpressionRefCount_Export(self.c_expr));
}

pub fn length(self: *const Expr) usize {
    return @intCast(wrtl.Length_Expression_Integer(self.c_expr));
}

pub fn part(self: *const Expr, i: isize) Expr {
    const c_expr = wrtl.Part_E_I_E(self.c_expr, i) orelse unreachable;
    return Expr.init(c_expr, false);
}

pub fn head(self: *const *Expr) Expr {
    const c_expr = wrtl.Part_E_I_E(self.c_expr, 0);
    return Expr.init(c_expr, false);
}

pub fn setPart(self: *Expr, i: usize, elem_expr: Expr) void {
    std.debug.assert(1 <= i and i <= self.length());
    wrtl.SetElement_EIE_E(self.c_expr, i, elem_expr.c_expr);
}

pub fn print(self: *const Expr) void {
    _ = wrtl.Print_E_I(self.c_expr);
    return;
}

pub fn eval(self: *const Expr) Expr {
    const c_expr = wrtl.Evaluate_E_E(self.c_expr);
    return Expr.wrap(c_expr);
}

pub fn sameQ(self: *const Expr, other: Expr) bool {
    return wrtl.SameQ_E_E_Boolean(self.c_expr, other.c_expr);
}

pub fn sameQStr(self: *const Expr, txt: [:0]const u8) bool {
    return self.sameQ(Expr.inertExpression(txt));
}

pub fn stringQ(self: *const Expr) bool {
    var bytes: [*:0]const u8 = undefined;
    return wrtl.TestGet_CString(self.c_expr, &bytes);
}

pub fn returnValue(self: *Expr) wrtl.OpaqueExpr {
    _ = self.acquire();
    return self.c_expr;
}

pub fn toUTF8(self: *const Expr) ?[]const u8 {
    var bytes: [*]const u8 = undefined;
    var len: wrtl.mint = undefined;
    if (!wrtl.StringExpressionToUTF8Bytes(self.c_expr, &bytes, &len)) {
        return null;
    }
    return bytes[0..@intCast(len)];
}

pub fn toInt(self: *const Expr) ?i64 {
    var res: i64 = undefined;
    if (!wrtl.TestGet_Integer(self.c_expr, 64, true, @ptrCast(&res))) {
        return null;
    }
    return res;
}

pub fn construct(expr_head: *const Expr, expr_args: anytype) Expr {
    // TODO: assert that expr_args is a tuple of Exprs
    std.debug.assert(checkTuple(@TypeOf(expr_args)));

    const args_len = expr_args.len;
    //var c_expr_args: std.meta.Tuple(&(.{wrtl.OpaqueExpr} ** args_len)) = undefined;

    const c_expr = wrtl.CreateHeaded_IE_E(args_len, expr_head.c_expr);
    inline for (expr_args, 0..) |expr_arg, i| {
        wrtl.SetElement_EIE_E(c_expr, i + 1, expr_arg.c_expr);
    }
    return Expr.wrap(c_expr);
}

fn checkTuple(comptime T: type) bool {
    const info = @typeInfo(T);
    if (info != .@"struct")
        @compileError("Expected struct type");
    if (!info.@"struct".is_tuple)
        @compileError("Struct type must be a tuple type");
    return true;
}

// Private methods
fn acquire(self: *Expr) usize {
    const count = wrtl.Expression_Acquire_Export(self.c_expr);
    return @intCast(count);
}

fn release(self: *Expr) usize {
    const count = wrtl.Expression_Release_Export(self.c_expr);
    return @intCast(count);
}

// Static methods
pub fn inertExpression(txt: [:0]const u8) Expr {
    const c_expr = wrtl.CreateGeneralExpr(txt.ptr);
    return Expr.init(c_expr, false);
}

pub fn failure() Expr {
    return Expr.inertExpression("$Failure");
}

pub fn embedObjectInstance(val: *anyopaque, name: [:0]const u8, head_expr: Expr) Expr {
    var init_instance: i32 = undefined;
    // Note: name needs to be null-terminated
    const c_expr = wrtl.Create_ObjectInstanceByNameInitWithHead(val, name.ptr, &init_instance, head_expr.c_expr);
    return Expr.init(c_expr, false);
}

pub fn unembedObjectInstance(self: Expr, className: [:0]const u8) ?*anyopaque {
    var obj: *anyopaque = undefined;
    if (wrtl.TestGet_ObjectInstanceByName(self.c_expr, className.ptr, &obj)) {
        return obj;
    }
    return null;
}

pub fn errorException(txt: []const u8) Expr {
    const body = Expr.inertExpression("List").construct(.{Expr.fromCString(txt)});
    const eThrow = Expr.inertExpression("CompileUtilities`Error`Exceptions`ThrowException").construct(.{body});
    return eThrow;
}

pub fn errorExceptionWithArg(txt: []const u8, arg1: Expr) Expr {
    const body = Expr.inertExpression("List").construct(.{Expr.fromCString(txt)});
    const eThrow = Expr.inertExpression("CompileUtilities`Error`Exceptions`ThrowException").construct(.{ body, arg1 });
    return eThrow;
}
