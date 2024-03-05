#include "compile.h"
#include "env.h"
#include "module.h"
#include "ops.h"
#include "symbol.h"
#include "univ/hash.h"
#include "univ/str.h"
#include "univ/system.h"

#define RegFree 0
#define RegEnv  1
#define RegTmp1 2
#define RegTmp2 3
#define RegTmp3 4

static u32 AllocFn;
static u32 FindVarFn;
static u32 MakePairFn;
static u32 MakeTupleFn;

static Result CompileFunc(Node *lambda, Frame *env, Module *mod);
static Result CompileExpr(Node *expr, Frame *env, ByteVec *bytes, Module *mod);

static void CompileConst(ByteVec *bytes, u32 num)
{
  PushByte(bytes, I32Const);
  PushInt(bytes, num);
}

static void CompileStore(ByteVec *bytes, u32 offset)
{
  CompileConst(bytes, 4);
  PushByte(bytes, I32Mul);
  PushByte(bytes, I32Store);
  PushInt(bytes, 1);
  PushInt(bytes, offset);
}

static void CompileLoad(ByteVec *bytes, u32 offset)
{
  CompileConst(bytes, 4);
  PushByte(bytes, I32Mul);
  PushByte(bytes, I32Load);
  PushInt(bytes, 1);
  PushInt(bytes, offset);
}

#define CompileHead(bytes)  CompileLoad(bytes, 0)
#define CompileTail(bytes)  CompileLoad(bytes, 1)

static void CompileCallOp(ByteVec *bytes, u32 typeidx)
{
  PushByte(bytes, CallIndirect);
  PushInt(bytes, 1);
  PushInt(bytes, typeidx);
}

static void GetReg(ByteVec *bytes, u32 reg)
{
  PushByte(bytes, GlobalGet);
  PushByte(bytes, reg);
}

static void SetReg(ByteVec *bytes, u32 reg)
{
  PushByte(bytes, GlobalSet);
  PushInt(bytes, reg);
}

#define PageSize 65536

static void CompileAlloc(ByteVec *bytes, u32 size)
{
  CompileConst(bytes, size);
  PushByte(bytes, CallOp);
  PushInt(bytes, AllocFn);
}

static void CompileDup(ByteVec *bytes)
{
  SetReg(bytes, RegTmp1);
  GetReg(bytes, RegTmp1);
  GetReg(bytes, RegTmp1);
}

static void CompileSwap(ByteVec *bytes)
{
  SetReg(bytes, RegTmp1);
  SetReg(bytes, RegTmp2);
  GetReg(bytes, RegTmp1);
  GetReg(bytes, RegTmp2);
}

static void CompileDig(ByteVec *bytes)
{
  SetReg(bytes, RegTmp1);
  SetReg(bytes, RegTmp2);
  SetReg(bytes, RegTmp3);
  GetReg(bytes, RegTmp2);
  GetReg(bytes, RegTmp1);
  GetReg(bytes, RegTmp3);
}

static void CompileBury(ByteVec *bytes)
{
  SetReg(bytes, RegTmp1);
  SetReg(bytes, RegTmp2);
  SetReg(bytes, RegTmp3);
  GetReg(bytes, RegTmp1);
  GetReg(bytes, RegTmp3);
  GetReg(bytes, RegTmp2);
}

static void CompileMakePair(ByteVec *bytes)
{
  PushByte(bytes, CallOp);
  PushInt(bytes, MakePairFn);
}

static void CompileMakeTuple(ByteVec *bytes, u32 size)
{
  CompileConst(bytes, size);
  PushByte(bytes, CallOp);
  PushInt(bytes, MakeTupleFn);
}

static void CompileMakeLambda(ByteVec *bytes, u32 funcidx)
{
  CompileConst(bytes, funcidx);
  GetReg(bytes, RegEnv);
  CompileMakePair(bytes);
}

static void CompileTupleLength(ByteVec *bytes)
{
  CompileLoad(bytes, 0);
}

static void ExtendEnv(ByteVec *bytes, u32 size)
{
  GetReg(bytes, RegEnv);
  CompileMakeTuple(bytes, size);
  CompileMakePair(bytes);
  SetReg(bytes, RegEnv);
}

static void CompileFindVar(ByteVec *bytes, Module *mod)
{
  PushByte(bytes, CallOp);
  PushInt(bytes, FindVarFn);
}

static void CompileLookup(ByteVec *bytes, Module *mod)
{
  CompileFindVar(bytes, mod);
  CompileLoad(bytes, 1);
}

static void CompileDefine(ByteVec *bytes, Module *mod)
{
  CompileFindVar(bytes, mod);
  CompileSwap(bytes);
  CompileStore(bytes, 1);
}

static Result CompileVar(Node *expr, Frame *env, ByteVec *bytes, Module *mod)
{
  u32 var = NodeValue(expr);
  i32 index = FrameFind(env, var);
  if (index < 0) return ErrorResult("Undefined variable", 0, expr->pos);

  CompileConst(bytes, index);
  CompileLookup(bytes, mod);

  return OkResult();
}

static Result CompileInt(Node *expr, Frame *env, ByteVec *bytes, Module *mod)
{
  PushByte(bytes, I32Const);
  PushInt(bytes, NodeValue(expr));
  return OkResult();
}

/* TODO: figure out how to handle unknown types */
static Result NumOp(Node *node)
{
  switch (node->type) {
  case AddNode: return ValueResult(I32Add);
  case SubNode: return ValueResult(I32Sub);
  case MulNode: return ValueResult(I32Mul);
  case DivNode: return ValueResult(I32DivS);
  case RemNode: return ValueResult(I32RemS);
  default:      return ErrorResult("Not a numeric expression", 0, node->pos);
  }
}

static Result CompileNumOp(Node *expr, Frame *env, ByteVec *bytes, Module *mod)
{
  Result result;

  result = CompileExpr(NodeChild(expr, 1), env, bytes, mod);
  if (!result.ok) return result;

  result = CompileExpr(NodeChild(expr, 0), env, bytes, mod);
  if (!result.ok) return result;

  result = NumOp(expr);
  if (!result.ok) return result;

  PushByte(bytes, ResultValue(result));
  return OkResult();
}

static Result CompilePair(Node *expr, Frame *env, ByteVec *bytes, Module *mod)
{
  Result result;

  result = CompileExpr(NodeChild(expr, 1), env, bytes, mod);
  if (!result.ok) return result;

  result = CompileExpr(NodeChild(expr, 0), env, bytes, mod);
  if (!result.ok) return result;

  CompileMakePair(bytes);

  return OkResult();
}

static Result CompileTuple(Node *expr, Frame *env, ByteVec *bytes, Module *mod)
{
  u32 i;
  u32 num_items = NumNodeChildren(expr);
  Result result;

  CompileMakeTuple(bytes, num_items);
  CompileDup(bytes);

  for (i = 0; i < num_items; i++) {
    CompileDup(bytes);

    result = CompileExpr(NodeChild(expr, i), env, bytes, mod);
    if (!result.ok) return result;

    CompileStore(bytes, 0);
    if (i < num_items - 1) {
      CompileConst(bytes, 1);
      PushByte(bytes, I32Add);
    } else {
      PushByte(bytes, DropOp);
    }
  }

  return OkResult();
}

static Result CompileLambda(Node *expr, Frame *env, ByteVec *bytes, Module *mod)
{
  Result result = CompileFunc(expr, env, mod);
  if (!result.ok) return result;

  CompileMakeLambda(bytes, ResultValue(result));
  return result;
}

static Result CompileCall(Node *expr, Frame *env, ByteVec *bytes, Module *mod)
{
  u32 i;
  Result result;
  u32 num_items = NumNodeChildren(expr);

  GetReg(bytes, RegEnv);
  for (i = 0; i < num_items; i++) {
    result = CompileExpr(NodeChild(expr, num_items - 1 - i), env, bytes, mod);
    if (!result.ok) return result;
  }

  CompileDup(bytes);
  CompileHead(bytes);
  SetReg(bytes, RegEnv);
  CompileTail(bytes);
  CompileCallOp(bytes, num_items);
  CompileSwap(bytes);
  SetReg(bytes, RegEnv);

  return OkResult();
}

static Result CompileExpr(Node *expr, Frame *env, ByteVec *bytes, Module *mod)
{
  switch (expr->type) {
  case IDNode:        return CompileVar(expr, env, bytes, mod);
  case IntNode:       return CompileInt(expr, env, bytes, mod);
  case AddNode:       return CompileNumOp(expr, env, bytes, mod);
  case SubNode:       return CompileNumOp(expr, env, bytes, mod);
  case MulNode:       return CompileNumOp(expr, env, bytes, mod);
  case DivNode:       return CompileNumOp(expr, env, bytes, mod);
  case RemNode:       return CompileNumOp(expr, env, bytes, mod);
  case PairNode:      return CompilePair(expr, env, bytes, mod);
  case TupleNode:     return CompileTuple(expr, env, bytes, mod);
  case LambdaNode:    return CompileLambda(expr, env, bytes, mod);
  case CallNode:      return CompileCall(expr, env, bytes, mod);
  default:            return ErrorResult("Unknown expr type", 0, NodePos(expr));
  }
}

static Result CompileFunc(Node *lambda, Frame *env, Module *mod)
{
  u32 i;
  Result result;
  Node *params = NodeChild(lambda, 0);
  u32 num_params = NumNodeChildren(params);
  u32 type = TypeIdx(mod, num_params, 1);
  Node *body = NodeChild(lambda, 1);
  Func *func = MakeFunc(type);
  env = ExtendFrame(env, num_params);

  for (i = 0; i < num_params; i++) {
    Node *param = NodeChild(params, i);
    FrameSet(env, i, NodeValue(param));
  }

  ExtendEnv(&func->code, num_params);
  for (i = 0; i < num_params; i++) {
    PushByte(&func->code, LocalGet);
    PushInt(&func->code, i);
    CompileConst(&func->code, i);
    CompileDefine(&func->code, mod);
  }

  result = CompileExpr(body, env, &func->code, mod);

  PopFrame(env);
  if (!result.ok) {
    FreeFunc(func);
    return result;
  }

  PushByte(&func->code, EndOp);

  ObjVecPush(&mod->funcs, func);

  return ValueResult(FuncIdx(mod, mod->funcs.count - 1));
}

static void CompileBaseFuncs(Module *mod)
{
  Func *alloc = MakeFunc(TypeIdx(mod, 1, 0));
  Func *findvar = MakeFunc(TypeIdx(mod, 1, 1));
  Func *makepair = MakeFunc(TypeIdx(mod, 2, 1));
  Func *maketuple = MakeFunc(TypeIdx(mod, 1, 1));
  u32 blocktype = TypeIdx(mod, 2, 2);

  ByteVec *code = &alloc->code;
  GetReg(code, RegFree);
  PushByte(code, LocalGet);
  PushInt(code, 0);
  PushByte(code, I32Add);
  SetReg(code, RegFree);
  PushByte(code, Block);
  PushInt(code, EmptyBlock);
  GetReg(code, RegFree);
  PushByte(code, MemSize);
  PushByte(code, 0);
  CompileConst(code, PageSize);
  PushByte(code, I32Mul);
  PushByte(code, I32LtS);
  PushByte(code, BrIf);
  PushInt(code, 0);
  CompileConst(code, 1);
  PushByte(code, MemGrow);
  PushByte(code, 0);
  PushByte(code, DropOp);
  PushByte(code, EndOp);
  PushByte(code, EndOp);
  ObjVecPush(&mod->funcs, alloc);
  AllocFn = FuncIdx(mod, mod->funcs.count-1);

  code = &findvar->code;
  PushByte(code, LocalGet);
  PushByte(code, 0);
  GetReg(code, RegEnv);
  PushByte(code, Block);
  PushInt(code, blocktype);
  PushByte(code, Loop);
  PushByte(code, blocktype);
  PushByte(code, Block);
  PushByte(code, blocktype);
  CompileDup(code);
  CompileConst(code, 0);
  PushByte(code, I32Ne);
  PushByte(code, BrIf);
  PushByte(code, 0);
  PushByte(code, Unreachable);
  PushByte(code, EndOp);
  CompileDup(code);
  CompileDig(code);
  CompileDup(code);
  CompileDig(code);
  CompileHead(code);
  CompileTupleLength(code);
  PushByte(code, I32LtS);
  PushByte(code, BrIf);
  PushInt(code, 1);
  CompileSwap(code);
  CompileDup(code);
  CompileTail(code);
  CompileSwap(code);
  CompileHead(code);
  CompileTupleLength(code);
  CompileDig(code);
  PushByte(code, I32Sub);
  CompileSwap(code);
  PushByte(code, Br);
  PushInt(code, 0);
  PushByte(code, EndOp);
  PushByte(code, EndOp);
  CompileSwap(code);
  CompileHead(code);
  PushByte(code, I32Add);

  PushByte(code, EndOp);
  ObjVecPush(&mod->funcs, findvar);
  FindVarFn = FuncIdx(mod, mod->funcs.count-1);

  code = &makepair->code;
  PushByte(code, LocalGet);
  PushByte(code, 0);
  PushByte(code, LocalGet);
  PushByte(code, 1);
  GetReg(code, RegFree);
  GetReg(code, RegFree);
  CompileAlloc(code, 2);
  CompileDig(code);
  CompileStore(code, 0);
  CompileDup(code);
  CompileDig(code);
  CompileStore(code, 1);

  PushByte(code, EndOp);
  ObjVecPush(&mod->funcs, makepair);
  MakePairFn = FuncIdx(mod, mod->funcs.count-1);

  code = &maketuple->code;
  PushByte(code, LocalGet);
  PushByte(code, 0);
  PushByte(code, LocalGet);
  PushByte(code, 0);
  GetReg(code, RegFree);
  GetReg(code, RegFree);
  CompileDig(code);
  PushByte(code, I32Add);
  CompileConst(code, 1);
  PushByte(code, I32Add);
  CompileDup(code);
  CompileConst(code, 2);
  PushByte(code, I32RemS);
  PushByte(code, I32Add);
  SetReg(code, RegFree);
  CompileDup(code);
  CompileDig(code);
  CompileStore(code, 0);
  CompileConst(code, 1);
  PushByte(code, I32Add);
  PushByte(code, EndOp);
  ObjVecPush(&mod->funcs, maketuple);
  MakeTupleFn = FuncIdx(mod, mod->funcs.count-1);
}

Result CompileModule(Node *ast)
{
  Module *mod = Alloc(sizeof(Module));
  u32 i, j;
  Node *stmts;
  Frame *env = 0;
  Result result;
  u32 num_exports = 0;
  u32 num_imports = 1;
  Func *start;

  if (ast->type != ModuleNode) return ErrorResult("Not a module", 0, 0);
  InitModule(mod);
  mod->num_globals = 5; /* env, free, tmp1, tmp2, tmp3 */

  AddImport(mod, "console.log", TypeIdx(mod, 1, 0));
  CompileBaseFuncs(mod);
  start = MakeFunc(TypeIdx(mod, 0, 0));

  stmts = NodeChild(ModuleBody(ast), 1);
  for (i = 0; i < NumNodeChildren(stmts); i++) {
    Node *stmt = NodeChild(stmts, i);
    if (stmt->type != DefNode) continue;
    num_exports++;
  }

  env = ExtendFrame(env, num_exports + num_imports);
  ExtendEnv(&start->code, num_exports + num_imports);

  FrameSet(env, 0, AddSymbol("print"));

  CompileMakeLambda(&start->code, 0);
  CompileConst(&start->code, 0);
  CompileDefine(&start->code, mod);

  /* define each func */
  for (i = 0; i < NumNodeChildren(stmts); i++) {
    Node *stmt = NodeChild(stmts, i);
    if (stmt->type != DefNode) continue;
    FrameSet(env, i, NodeValue(NodeChild(stmt, 0)));
  }

  /* compile each func */
  for (i = 0; i < NumNodeChildren(stmts); i++) {
    Node *stmt = NodeChild(stmts, i);
    Node *lambda;
    Export *export;
    char *name;

    if (stmt->type != DefNode) continue;

    lambda = NodeChild(stmt, 1);
    result = CompileFunc(lambda, env, mod);
    if (!result.ok) return result;

    CompileMakeLambda(&start->code, ResultValue(result));
    CompileConst(&start->code, i + num_imports);
    CompileDefine(&start->code, mod);

    name = SymbolName(NodeValue(NodeChild(stmt, 0)));
    export = MakeExport(name, ResultValue(result));
    ObjVecPush(&mod->exports, export);
  }

  /* compile start func */
  for (i = 0, j = 0; i < NumNodeChildren(stmts); i++) {
    Node *stmt = NodeChild(stmts, i);

    if (stmt->type == DefNode) continue;

    result = CompileExpr(stmt, env, &start->code, mod);
    if (!result.ok) return result;

    PushByte(&start->code, DropOp);
  }
  PushByte(&start->code, EndOp);

  ObjVecPush(&mod->funcs, start);
  mod->start = FuncIdx(mod, mod->funcs.count - 1);

  return ItemResult(mod);
}
