pub const OpaqueExpr = *anyopaque;

// typedef int mbool;
pub const mbool = c_int;

pub const mint = isize;
pub const umint = usize;

// mint Length_Expression_Integer(void* arg);
pub extern fn Length_Expression_Integer(expr: OpaqueExpr) mint;
// void* Part_E_I_E(void* arg, mint index);
pub extern fn Part_E_I_E(expr: OpaqueExpr, index: mint) ?OpaqueExpr;
// void SetElement_EIE_E(void* base_arg, mint pos, void* elem_arg);
pub extern fn SetElement_EIE_E(base_expr: OpaqueExpr, pos: umint, elem_expr: OpaqueExpr) void;
// void* Expression_SetPart_Export( void* arg1, void* arg2, void* arg3, mbool* err);
pub extern fn Expression_SetPart_Export(expr: OpaqueExpr, pos: *anyopaque, elem: OpaqueExpr, err: *bool) *anyopaque;
// mint Expression_Acquire_Export(void* arg1);
pub extern fn Expression_Acquire_Export(expr: OpaqueExpr) mint;
// mint Expression_Release_Export(void* arg1);
pub extern fn Expression_Release_Export(expr: OpaqueExpr) mint;
// mint Print_E_I(void* arg);
pub extern fn Print_E_I(expr: OpaqueExpr) mint;
// void* Evaluate_E_E(void* arg);
pub extern fn Evaluate_E_E(expr: OpaqueExpr) OpaqueExpr;
// extern "C" void* CreateGeneralExpr(const char*);
pub extern fn CreateGeneralExpr(cstr: [*:0]const u8) OpaqueExpr;
// extern "C" void* CreateHeaded_IE_E(mint, void*);
pub extern fn CreateHeaded_IE_E(len: umint, expr: OpaqueExpr) OpaqueExpr;
// extern "C" bool SameQ_E_E_Boolean(void*, void*);
pub extern fn SameQ_E_E_Boolean(expr1: OpaqueExpr, expr2: OpaqueExpr) bool;
// extern "C" void* UTF8BytesAndLengthToStringExpression(const char*, mint, mint);
pub extern fn UTF8BytesAndLengthToStringExpression(data: [*]const u8, nbytes: mint, len: mint) OpaqueExpr;
/// Convert an LLVM integer to an expr.
///
/// extern "C" void* CreateIntegerExpr(void*, mint, bool);
pub extern fn CreateIntegerExpr(src: *anyopaque, size: mint, signedQ: bool) OpaqueExpr;

// void* Create_ObjectInstanceByNameInitWithHead(object_t inst, char* className, mbool*init, void *vhead);
pub extern fn Create_ObjectInstanceByNameInitWithHead(inst: *anyopaque, className: [*:0]const u8, init: *mbool, vhead: OpaqueExpr) OpaqueExpr;
// bool TestGet_ObjectInstanceByName(void* arg, char* className, object_t* res);
pub extern fn TestGet_ObjectInstanceByName(expr: OpaqueExpr, className: [*:0]const u8, ptr: **anyopaque) bool;
// bool StringExpressionToUTF8Bytes(void* arg, unsigned char** dataP, mint* lenP);
pub extern fn StringExpressionToUTF8Bytes(str_expr: OpaqueExpr, data: *[*]const u8, len: *mint) bool;
// bool TestGet_CString(void* arg, char** res);
pub extern fn TestGet_CString(expr: OpaqueExpr, cstr_ptr: *[*:0]const u8) bool;
/// Test get from an expr into a native LLVM integer type.
/// Directly write result into destination memory.
///
/// bool TestGet_Integer(void *arg, const uint32_t size, const bool signedQ, void *res);
pub extern fn TestGet_Integer(int_expr: OpaqueExpr, size: u32, signedQ: bool, res: *anyopaque) bool;

// int ExpressionType_Export(void *arg);
pub extern fn ExpressionType_Export(expr: OpaqueExpr) c_int;
// rctype ExpressionRefCount_Export(void *arg);
pub extern fn ExpressionRefCount_Export(expr: OpaqueExpr) u32;

// mint InitializeCompilerClass_Export(char* name);
pub extern fn InitializeCompilerClass_Export(className: [*:0]const u8) mint;
// mint AddCompilerClassMethod_Export(char* className, char* methodName, void* fun);
pub extern fn AddCompilerClassMethod_Export(className: [*:0]const u8, methodName: [*:0]const u8, fun: *anyopaque) mint;
// mint FinalizeCompilerClass_Export(char* className);
pub extern fn FinalizeCompilerClass_Export(className: [*:0]const u8) mint;
// umint SetClassRawMethod(char* className, char* methodName, void* fun);
pub extern fn SetClassRawMethod(className: [*:0]const u8, methodName: [*:0]const u8, fun: *anyopaque) umint;
