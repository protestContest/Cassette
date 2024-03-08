#include "compile.h"
#include "env.h"
#include "module.h"
#include "ops.h"
#include "symbol.h"
#include "univ/hash.h"
#include "univ/str.h"
#include "univ/system.h"
#include <stdio.h>

#define RegFree 0
#define RegEnv  1
#define RegTmp1 2
#define RegTmp2 3
#define RegTmp3 4

static u32 AllocFn;
static u32 LookupFn;
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
  PushByte(bytes, I32Store);
  PushInt(bytes, 2);
  PushInt(bytes, offset);
}

static void CompileLoad(ByteVec *bytes, u32 offset)
{
  PushByte(bytes, I32Load);
  PushInt(bytes, 2);
  PushInt(bytes, offset);
}

#define CompileHead(bytes)  CompileLoad(bytes, 0)
#define CompileTail(bytes)  CompileLoad(bytes, 4)

static void CompileCallOp(ByteVec *bytes, u32 typeidx)
{
  PushByte(bytes, CallIndirect);
  PushInt(bytes, typeidx);
  PushInt(bytes, 0);
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

static void GetLocal(ByteVec *bytes, u32 local)
{
  PushByte(bytes, LocalGet);
  PushInt(bytes, local);
}

static void SetLocal(ByteVec *bytes, u32 local)
{
  PushByte(bytes, LocalSet);
  PushInt(bytes, local);
}

static void TeeLocal(ByteVec *bytes, u32 local)
{
  PushByte(bytes, LocalTee);
  PushInt(bytes, local);
}

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

static void CompileLookup(ByteVec *bytes, Module *mod)
{
  PushByte(bytes, CallOp);
  PushInt(bytes, LookupFn);
}

static Result CompileVar(Node *expr, Frame *env, ByteVec *bytes, Module *mod)
{
  u32 var = NodeValue(expr);
  i32 index = FrameFind(env, var);
  printf("Var %s\n", SymbolName(var));
  if (index < 0) return ErrorResult("Undefined variable", mod->filename, expr->pos);

  CompileConst(bytes, index);
  CompileLookup(bytes, mod);

  return OkResult();
}

static Result CompileInt(Node *expr, Frame *env, ByteVec *bytes, Module *mod)
{
  CompileConst(bytes, NodeValue(expr));
  return OkResult();
}

static OpCode NodeOp(Node *node)
{
  switch (node->type) {
  case AddNode:     return I32Add;
  case SubNode:     return I32Sub;
  case MulNode:     return I32Mul;
  case DivNode:     return I32DivS;
  case RemNode:     return I32RemS;
  case LtNode:      return I32LtS;
  case LtEqNode:    return I32LeS;
  case GtNode:      return I32GtS;
  case GtEqNode:    return I32GeS;
  case EqNode:      return I32Eq;
  case NotEqNode:   return I32Ne;
  default:          return Unreachable;
  }
}

static Result CompileNeg(Node *expr, Frame *env, ByteVec *bytes, Module *mod)
{
  Result result;

  CompileConst(bytes, 0);

  result = CompileExpr(NodeChild(expr, 0), env, bytes, mod);
  if (!result.ok) return result;

  PushByte(bytes, I32Sub);
  return OkResult();
}

static Result CompileNot(Node *expr, Frame *env, ByteVec *bytes, Module *mod)
{
  Result result = CompileExpr(NodeChild(expr, 0), env, bytes, mod);
  if (!result.ok) return result;

  PushByte(bytes, I32EqZ);
  return OkResult();
}

static Result CompileBinOp(Node *expr, Frame *env, ByteVec *bytes, Module *mod)
{
  Result result;

  result = CompileExpr(NodeChild(expr, 1), env, bytes, mod);
  if (!result.ok) return result;

  result = CompileExpr(NodeChild(expr, 0), env, bytes, mod);
  if (!result.ok) return result;

  PushByte(bytes, NodeOp(expr));
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

  SetReg(bytes, RegTmp1);
  GetReg(bytes, RegTmp1);
  CompileHead(bytes);
  SetReg(bytes, RegEnv);
  GetReg(bytes, RegTmp1);
  CompileTail(bytes);
  CompileCallOp(bytes, TypeIdx(mod, num_items-1, 1));
  SetReg(bytes, RegTmp1);
  SetReg(bytes, RegEnv);
  GetReg(bytes, RegTmp1);

  return OkResult();
}

static Result CompileExpr(Node *expr, Frame *env, ByteVec *bytes, Module *mod)
{
  printf("Compiling %s (%d)\n", NodeName(expr), expr->pos);

  switch (expr->type) {
  case IDNode:        return CompileVar(expr, env, bytes, mod);
  case IntNode:       return CompileInt(expr, env, bytes, mod);
  case AddNode:
  case SubNode:
  case MulNode:
  case DivNode:
  case RemNode:       return CompileBinOp(expr, env, bytes, mod);
  /*case LenNode:       return CompileLen(expr, env, bytes, mod);*/
  case NotNode:       return CompileNot(expr, env, bytes, mod);
  case NegNode:       return CompileNeg(expr, env, bytes, mod);
  case PairNode:      return CompilePair(expr, env, bytes, mod);
  case TupleNode:     return CompileTuple(expr, env, bytes, mod);
  case LambdaNode:    return CompileLambda(expr, env, bytes, mod);
  case CallNode:      return CompileCall(expr, env, bytes, mod);
  default:            return ErrorResult("Unknown expr type", mod->filename, NodePos(expr));
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
  Func *func = MakeFunc(type, 0);
  env = ExtendFrame(env, num_params);

  for (i = 0; i < num_params; i++) {
    Node *param = NodeChild(params, i);
    FrameSet(env, i, NodeValue(param));
  }

  ExtendEnv(&func->code, num_params);
  GetReg(&func->code, RegEnv);
  CompileLoad(&func->code, 0);
  SetReg(&func->code, RegTmp1);
  for (i = 0; i < num_params; i++) {
    GetReg(&func->code, RegTmp1);
    GetLocal(&func->code, i);
    CompileStore(&func->code, i*4);
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

#define PageSize 65536
static void CompileUtilities(Module *mod)
{
  Func *alloc = MakeFunc(TypeIdx(mod, 1, 0), 0);
  Func *lookup = MakeFunc(TypeIdx(mod, 1, 1), 2);
  Func *makepair = MakeFunc(TypeIdx(mod, 2, 1), 0);
  Func *maketuple = MakeFunc(TypeIdx(mod, 1, 1), 0);

  ByteVec *code = &alloc->code;
  GetReg(code, RegFree);
  GetLocal(code, 0);
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

  code = &lookup->code;
  GetReg(code, RegEnv);
  SetLocal(code, 1);
  PushByte(code, Block);
  PushByte(code, EmptyBlock);
  PushByte(code, Loop);
  PushByte(code, EmptyBlock);
  /* assert env != 0 */
  PushByte(code, Block);
  PushByte(code, EmptyBlock);
  GetLocal(code, 1);
  CompileConst(code, 0);
  PushByte(code, I32Ne);
  PushByte(code, BrIf);
  PushByte(code, 0);
  PushByte(code, Unreachable);
  PushByte(code, EndOp);
  GetLocal(code, 0);
  GetLocal(code, 1);
  CompileLoad(code, 0);
  CompileConst(code, 4);
  PushByte(code, I32Sub);
  CompileLoad(code, 0);
  TeeLocal(code, 2);
  PushByte(code, I32LtS);
  PushByte(code, BrIf);
  PushInt(code, 1);
  GetLocal(code, 0);
  GetLocal(code, 2);
  PushByte(code, I32Sub);
  SetLocal(code, 0);
  GetLocal(code, 1);
  CompileLoad(code, 4);
  SetLocal(code, 1);
  PushByte(code, Br);
  PushInt(code, 0);
  PushByte(code, EndOp);
  PushByte(code, EndOp);
  GetLocal(code, 1);
  CompileLoad(code, 0);
  GetLocal(code, 0);
  CompileConst(code, 4);
  PushByte(code, I32Mul);
  PushByte(code, I32Add);
  CompileLoad(code, 0);
  PushByte(code, EndOp);
  ObjVecPush(&mod->funcs, lookup);
  LookupFn = FuncIdx(mod, mod->funcs.count-1);

  code = &makepair->code;
  GetReg(code, RegFree);
  GetReg(code, RegFree);
  GetReg(code, RegFree);
  CompileAlloc(code, 8);
  GetLocal(code, 1);
  CompileStore(code, 0);
  GetLocal(code, 0);
  CompileStore(code, 4);
  PushByte(code, EndOp);
  ObjVecPush(&mod->funcs, makepair);
  MakePairFn = FuncIdx(mod, mod->funcs.count-1);

  code = &maketuple->code;
  /* store count */
  GetReg(code, RegFree);
  PushByte(code, LocalGet);
  PushInt(code, 0);
  CompileAlloc(code, 4);
  CompileStore(code, 0);
  /* tuple pointer to return */
  GetReg(code, RegFree);
  /* allocate space for items */
  GetLocal(code, 0);
  CompileConst(code, 4);
  PushByte(code, I32Mul);
  PushByte(code, CallOp);
  PushInt(code, AllocFn);
  PushByte(code, EndOp);
  ObjVecPush(&mod->funcs, maketuple);
  MakeTupleFn = FuncIdx(mod, mod->funcs.count-1);
}

void CompileImports(Frame *env, Func *start, Module *mod)
{
  u32 i;
  ByteVec *code = &start->code;

  /* load current frame to tmp1 */
  GetReg(code, RegEnv);
  CompileLoad(code, 0);
  SetReg(code, RegTmp1);

  for (i = 0; i < mod->imports.count; i++) {
    Import *import = VecRef(&mod->imports, i);
    FrameSet(env, i, AddSymbol(import->name));
    GetReg(code, RegTmp1);
    CompileMakeLambda(code, i);
    CompileStore(code, i*4);
  }
}

Result CompileModule(Node *ast)
{
  Module *mod = Alloc(sizeof(Module));
  u32 i, j;
  Node *stmts;
  Frame *env = 0;
  Result result;
  u32 num_exports = 0;
  Func *start;
  Node *exports;

  if (ast->type != ModuleNode) return ErrorResult("Not a module", 0, 0);
  exports = ModuleExports(ast);
  num_exports = NumNodeChildren(exports);
  stmts = NodeChild(ModuleBody(ast), 1);

  InitModule(mod);
  mod->num_globals = 5; /* env, free, tmp1, tmp2, tmp3 */
  mod->filename = SymbolName(ModuleFile(ast));

  /* imports must be added first */
  AddImport(mod, "print", TypeIdx(mod, 1, 1));
  CompileUtilities(mod);

  start = MakeFunc(TypeIdx(mod, 0, 0), 0);
  env = ExtendFrame(env, mod->imports.count);
  ExtendEnv(&start->code, mod->imports.count);
  CompileImports(env, start, mod);

  /* pre-define each func */
  if (num_exports > 0) {
    env = ExtendFrame(env, num_exports);
    ExtendEnv(&start->code, num_exports);
    for (i = 0; i < NumNodeChildren(stmts); i++) {
      Node *stmt = NodeChild(stmts, i);
      if (stmt->type != DefNode) continue;
      FrameSet(env, i, NodeValue(NodeChild(stmt, 0)));
    }

    PrintEnv(env);

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

      GetReg(&start->code, RegEnv);
      CompileLoad(&start->code, 0);
      CompileMakeLambda(&start->code, ResultValue(result));
      CompileStore(&start->code, i*4);

      name = SymbolName(NodeValue(NodeChild(stmt, 0)));
      export = MakeExport(name, ResultValue(result));
      ObjVecPush(&mod->exports, export);
    }
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
