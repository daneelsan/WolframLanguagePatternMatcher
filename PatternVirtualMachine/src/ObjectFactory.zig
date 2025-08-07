const Expr = @import("Expr.zig");
//const AST = @import("AST.zig");
const wrtl = @import("WolframRTL.zig");

const std = @import("std");

export fn InstantiateObject(arg: wrtl.OpaqueExpr) wrtl.OpaqueExpr {
    var val = Expr.init(arg, true);
    defer val.deinit();

    if (val.length() < 2) {
        var expr_exception = Expr.errorException("No argument passed to InstantiateObject.");
        return expr_exception.returnValue();
    }

    var arg_expr = val.part(2);
    defer arg_expr.deinit();
    arg_expr.print();

    // TODO: Check why arg_expr.refcount() crashes
    Expr.inertExpression("System`Private`GetRefCount").construct(.{arg_expr}).eval().print();

    if (arg_expr.sameQStr("\"MExprFactory\"")) {
        arg_expr.print();
        //return AST.MExprFactory().returnValue();
        //var expr = Expr.inertExpression("testXXXX");
        var expr = Expr.inertExpression("foo").construct(.{Expr.inertExpression("bar")});
        return expr.returnValue();
    }

    {
        var tmp = Expr.errorExceptionWithArg("It is not known how to instantiate `1`.", arg_expr);
        return tmp.returnValue();
    }
}
