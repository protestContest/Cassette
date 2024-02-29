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
  char *var = NodeValue(expr);
  i32 index = FrameFind(env, Hash(var, StrLen(var)));
  if (index < 0) return ErrorResult("Undefined variable", 0, expr->pos);

  ByteVecPush(bytes, LocalGet);
  PushInt(bytes, index);

  return OkResult();
}

static Result CompileInt(Node *expr, Frame *env, ByteVec *bytes)
{
  ByteVecPush(bytes, I32Const);
  PushInt(bytes, expr->expr.intval);
  return OkResult();
}

static Result CompileFloat(Node *expr, Frame *env, ByteVec *bytes)
{
  ByteVecPush(bytes, F32Const);
  PushWord(bytes, expr->expr.floatval);
  return OkResult();
}

static Result CompileBinOp(OpCode op, Node *expr, Frame *env, ByteVec *bytes)
{
  Result result;
  result = CompileExpr(NodeChild(expr, 0), env, bytes);
  if (!result.ok) return result;
  result = CompileExpr(NodeChild(expr, 1), env, bytes);
  if (!result.ok) return result;
  ByteVecPush(bytes, op);
  return OkResult();
}

static Result CompileExpr(Node *expr, Frame *env, ByteVec *bytes)
{
  switch (expr->type) {
  case IDNode:    return CompileVar(expr, env, bytes);
  case IntNode:   return CompileInt(expr, env, bytes);
  case FloatNode: return CompileFloat(expr, env, bytes);
  case AddNode:   return CompileBinOp(I32Add, expr, env, bytes);
  case SubNode:   return CompileBinOp(I32Sub, expr, env, bytes);
  case MulNode:   return CompileBinOp(I32Mul, expr, env, bytes);
  case DivNode:   return CompileBinOp(I32DivS, expr, env, bytes);
  case RemNode:   return CompileBinOp(I32RemS, expr, env, bytes);
  default:        return ErrorResult("Unknown expr type", 0, NodePos(expr));
  }
}

static Result CompileFunc(Node *lambda)
{
  u32 i;
  Result result;
  Node *params = NodeChild(lambda, 0);
  Node *body = NodeChild(lambda, 1);
  Func *func = MakeFunc(NumNodeChildren(params));
  Frame *env = ExtendFrame(0, NumNodeChildren(params));

  for (i = 0; i < NumNodeChildren(params); i++) {
    char *var = NodeValue(NodeChild(params, i));
    FrameSet(env, i, Hash(var, StrLen(var)));
  }

  result = CompileExpr(body, env, &func->code);
  FreeEnv(env);
  if (!result.ok) {
    FreeFunc(func);
    return result;
  }

  ByteVecPush(&func->code, 0x0B); /* end byte */

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
