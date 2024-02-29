#include "compile.h"
#include "env.h"
#include "module.h"
#include "ops.h"
#include "univ/hash.h"
#include "univ/str.h"
#include "univ/system.h"

static Result CompileExpr(Node *expr, Frame *env, ByteVec *bytes);

static Result CompileVar(Node *expr, Frame *env, ByteVec *bytes)
{
  char *name = NodeValue(expr);
  u32 var = Hash(name, StrLen(name));
  i32 index = FrameFind(env, var);
  ValType type;
  if (index < 0) return ErrorResult("Undefined variable", 0, expr->pos);
  type = FrameFindType(env, var);

  ByteVecPush(bytes, LocalGet);
  PushInt(bytes, index);

  return ValueResult(type);
}

static Result CompileInt(Node *expr, Frame *env, ByteVec *bytes)
{
  ByteVecPush(bytes, I32Const);
  PushInt(bytes, expr->expr.intval);
  return ValueResult(I32);
}

static Result CompileFloat(Node *expr, Frame *env, ByteVec *bytes)
{
  ByteVecPush(bytes, F32Const);
  PushWord(bytes, expr->expr.floatval);
  return ValueResult(F32);
}

/* TODO: figure out how to handle unknown types */
static Result NumOp(ValType val_type, Node *node)
{
  switch (node->type) {
  case AddNode:
    switch (val_type) {
    case I32:   return ValueResult(I32Add);
    case F32:   return ValueResult(F32Add);
    default:    return ValueResult(I32Add);
    }
  case SubNode:
    switch (val_type) {
    case I32:   return ValueResult(I32Sub);
    case F32:   return ValueResult(F32Sub);
    default:    return ValueResult(I32Sub);
    }
  case MulNode:
    switch (val_type) {
    case I32:   return ValueResult(I32Mul);
    case F32:   return ValueResult(F32Mul);
    default:    return ValueResult(I32Mul);
    }
  case DivNode:
    switch (val_type) {
    case I32:   return ValueResult(I32DivS);
    case F32:   return ValueResult(F32Div);
    default:    return ValueResult(I32DivS);
    }
  case RemNode:
    switch (val_type) {
    case I32:   return ValueResult(I32RemS);
    case F32:   return ErrorResult("Type error", 0, node->pos);
    default:    return ValueResult(I32RemS);
    }
  default:    return ErrorResult("Not a numeric expression", 0, node->pos);
  }
}

static Result CompileNumOp(Node *expr, Frame *env, ByteVec *bytes)
{
  Result result;
  ValType a, b, res_type;
  u32 index;

  result = CompileExpr(NodeChild(expr, 0), env, bytes);
  if (!result.ok) return result;
  a = ResultValue(result);
  index = bytes->count;

  result = CompileExpr(NodeChild(expr, 1), env, bytes);
  if (!result.ok) return result;
  b = ResultValue(result);

  if (a == I32 && b == F32) {
    InsertByte(bytes, index, F32ConvertI32S);
    res_type = F32;
  } else if (a == F32 && b == I32) {
    PushByte(bytes, F32ConvertI32S);
    res_type = F32;
  } else {
    res_type = a;
  }

  result = NumOp(res_type, expr);
  if (!result.ok) return result;

  ByteVecPush(bytes, ResultValue(result));
  return ValueResult(res_type);
}

static Result CompileExpr(Node *expr, Frame *env, ByteVec *bytes)
{
  switch (expr->type) {
  case IDNode:    return CompileVar(expr, env, bytes);
  case IntNode:   return CompileInt(expr, env, bytes);
  case FloatNode: return CompileFloat(expr, env, bytes);
  case AddNode:   return CompileNumOp(expr, env, bytes);
  case SubNode:   return CompileNumOp(expr, env, bytes);
  case MulNode:   return CompileNumOp(expr, env, bytes);
  case DivNode:   return CompileNumOp(expr, env, bytes);
  case RemNode:   return CompileNumOp(expr, env, bytes);
  default:        return ErrorResult("Unknown expr type", 0, NodePos(expr));
  }
}

Result CompileFunc(Node *lambda)
{
  u32 i;
  Result result;
  Node *params = NodeChild(lambda, 0);
  Node *body = NodeChild(lambda, 1);
  Func *func = MakeFunc(NumNodeChildren(params));
  Frame *env = ExtendFrame(0, NumNodeChildren(params));

  for (i = 0; i < NumNodeChildren(params); i++) {
    char *var = NodeValue(NodeChild(params, i));
    FrameSet(env, i, AnyType, Hash(var, StrLen(var)));
  }

  result = CompileExpr(body, env, &func->code);
  FreeEnv(env);
  if (!result.ok) {
    FreeFunc(func);
    return result;
  }

  ByteVecPush(&func->code, EndOp);

  return ItemResult(func);
}

Result CompileModule(Node *ast)
{
  Module *mod = Alloc(sizeof(Module));
  u32 i;
  Node *stmts;
  Export *export;
  Result result;
  if (ast->type != ModuleNode) return ErrorResult("Not a module", 0, 0);
  InitModule(mod);

  stmts = NodeChild(ModuleBody(ast), 1);
  for (i = 0; i < NumNodeChildren(stmts); i++) {
    Node *stmt = NodeChild(stmts, i);
    Node *lambda;
    char *name;

    if (stmt->type != DefNode) continue;

    lambda = NodeChild(stmt, 1);

    result = CompileFunc(lambda);
    if (!result.ok) return result;
    ObjVecPush(&mod->funcs, ResultItem(result));

    name = NodeValue(NodeChild(stmt, 0));
    export = MakeExport(name, mod->funcs.count - 1);
    ObjVecPush(&mod->exports, export);
  }

  return ItemResult(mod);
}
