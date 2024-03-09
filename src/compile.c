#include "compile.h"
#include "builderror.h"
#include "env.h"
#include "module.h"
#include "wasm.h"
#include <univ.h>

#define RegFree 0
#define RegEnv  1
#define RegTmp1 2
#define RegTmp2 3
#define RegTmp3 4

typedef struct {
  Module *mod;
  Frame *env;
  u8 **bytes;
} Compiler;

static u32 AllocFn;
static u32 LookupFn;
static u32 MakePairFn;
static u32 MakeTupleFn;

static Result CompileFunc(Node *lambda, Frame *env, HashMap *imports, Module *mod);
static Result CompileExpr(Node *expr, u8 **bytes, Frame *env, HashMap *imports, Module *mod);

#define CompileError(msg, file, pos) Error(MakeBuildError(msg, file, pos))

static void Const(u8 **bytes, i32 num)
{
  PushByte(bytes, I32Const);
  PushInt(bytes, num);
}

static void Store(u8 **bytes, i32 offset)
{
  PushByte(bytes, I32Store);
  PushInt(bytes, 2);
  PushInt(bytes, offset);
}

static void Load(u8 **bytes, i32 offset)
{
  PushByte(bytes, I32Load);
  PushInt(bytes, 2);
  PushInt(bytes, offset);
}

#define Head(bytes)  Load(bytes, 0)
#define Tail(bytes)  Load(bytes, 4)

static void Call(u8 **bytes, i32 typeidx)
{
  PushByte(bytes, CallIndirect);
  PushInt(bytes, typeidx);
  PushInt(bytes, 0);
}

static void GetReg(u8 **bytes, i32 reg)
{
  PushByte(bytes, GlobalGet);
  PushByte(bytes, reg);
}

static void SetReg(u8 **bytes, i32 reg)
{
  PushByte(bytes, GlobalSet);
  PushInt(bytes, reg);
}

static void GetLocal(u8 **bytes, i32 local)
{
  PushByte(bytes, LocalGet);
  PushInt(bytes, local);
}

static void SetLocal(u8 **bytes, i32 local)
{
  PushByte(bytes, LocalSet);
  PushInt(bytes, local);
}

static void TeeLocal(u8 **bytes, i32 local)
{
  PushByte(bytes, LocalTee);
  PushInt(bytes, local);
}

static void Alloc(u8 **bytes, i32 size)
{
  Const(bytes, size);
  PushByte(bytes, CallOp);
  PushInt(bytes, AllocFn);
}

static void Dup(u8 **bytes)
{
  SetReg(bytes, RegTmp1);
  GetReg(bytes, RegTmp1);
  GetReg(bytes, RegTmp1);
}

static void Swap(u8 **bytes)
{
  SetReg(bytes, RegTmp1);
  SetReg(bytes, RegTmp2);
  GetReg(bytes, RegTmp1);
  GetReg(bytes, RegTmp2);
}

static void Dig(u8 **bytes)
{
  SetReg(bytes, RegTmp1);
  SetReg(bytes, RegTmp2);
  SetReg(bytes, RegTmp3);
  GetReg(bytes, RegTmp2);
  GetReg(bytes, RegTmp1);
  GetReg(bytes, RegTmp3);
}

static void Bury(u8 **bytes)
{
  SetReg(bytes, RegTmp1);
  SetReg(bytes, RegTmp2);
  SetReg(bytes, RegTmp3);
  GetReg(bytes, RegTmp1);
  GetReg(bytes, RegTmp3);
  GetReg(bytes, RegTmp2);
}

static void MakePair(u8 **bytes)
{
  PushByte(bytes, CallOp);
  PushInt(bytes, MakePairFn);
}

static void MakeTuple(u8 **bytes, i32 size)
{
  Const(bytes, size);
  PushByte(bytes, CallOp);
  PushInt(bytes, MakeTupleFn);
}

static void MakeLambda(u8 **bytes, i32 funcidx)
{
  Const(bytes, funcidx);
  GetReg(bytes, RegEnv);
  MakePair(bytes);
}

static void ExtendEnv(u8 **bytes, i32 size)
{
  GetReg(bytes, RegEnv);
  MakeTuple(bytes, size);
  MakePair(bytes);
  SetReg(bytes, RegEnv);
}

static void Lookup(u8 **bytes, Module *mod)
{
  PushByte(bytes, CallOp);
  PushInt(bytes, LookupFn);
}

static Result CompileVar(Node *expr, u8 **bytes, Frame *env, HashMap *imports, Module *mod)
{
  u32 var = NodeValue(expr);
  i32 index = FrameFind(env, var);
  printf("Var %s\n", SymbolName(var));
  if (index < 0) return CompileError("Undefined variable", mod->filename, expr->pos);

  Const(bytes, index);
  Lookup(bytes, mod);

  return Ok(0);
}

static Result CompileInt(Node *expr, u8 **bytes, Frame *env, HashMap *imports, Module *mod)
{
  Const(bytes, NodeValue(expr));
  return Ok(0);
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

static Result CompileNeg(Node *expr, u8 **bytes, Frame *env, HashMap *imports, Module *mod)
{
  Result result;

  Const(bytes, 0);

  result = CompileExpr(NodeChild(expr, 0), bytes, env, imports, mod);
  if (!IsOk(result)) return result;

  PushByte(bytes, I32Sub);
  return Ok(0);
}

static Result CompileNot(Node *expr, u8 **bytes, Frame *env, HashMap *imports, Module *mod)
{
  Result result = CompileExpr(NodeChild(expr, 0), bytes, env, imports, mod);
  if (!IsOk(result)) return result;

  PushByte(bytes, I32EqZ);
  return Ok(0);
}

static Result CompileBinOp(Node *expr, u8 **bytes, Frame *env, HashMap *imports, Module *mod)
{
  Result result;

  result = CompileExpr(NodeChild(expr, 1), bytes, env, imports, mod);
  if (!IsOk(result)) return result;

  result = CompileExpr(NodeChild(expr, 0), bytes, env, imports, mod);
  if (!IsOk(result)) return result;

  PushByte(bytes, NodeOp(expr));
  return Ok(0);
}

static Result CompilePair(Node *expr, u8 **bytes, Frame *env, HashMap *imports, Module *mod)
{
  Result result;

  result = CompileExpr(NodeChild(expr, 1), bytes, env, imports, mod);
  if (!IsOk(result)) return result;

  result = CompileExpr(NodeChild(expr, 0), bytes, env, imports, mod);
  if (!IsOk(result)) return result;

  MakePair(bytes);

  return Ok(0);
}

static Result CompileList(Node *expr, u8 **bytes, Frame *env, HashMap *imports, Module *mod)
{
  Result result;
  u32 i;
  u32 len = NodeCount(expr);

  Const(bytes, 0);
  for (i = 0; i < len; i++) {
    result = CompileExpr(NodeChild(expr, len - 1 - i), bytes, env, imports, mod);
    if (!IsOk(result)) return result;
    MakePair(bytes);
  }

  return Ok(0);
}

static Result CompileTuple(Node *expr, u8 **bytes, Frame *env, HashMap *imports, Module *mod)
{
  u32 i;
  u32 num_items = NodeCount(expr);
  Result result;

  MakeTuple(bytes, num_items);
  Dup(bytes);

  for (i = 0; i < num_items; i++) {
    Dup(bytes);

    result = CompileExpr(NodeChild(expr, i), bytes, env, imports, mod);
    if (!IsOk(result)) return result;

    Store(bytes, 0);
    if (i < num_items - 1) {
      Const(bytes, 1);
      PushByte(bytes, I32Add);
    } else {
      PushByte(bytes, DropOp);
    }
  }

  return Ok(0);
}

static Result CompileLambda(Node *expr, u8 **bytes, Frame *env, HashMap *imports, Module *mod)
{
  Result result = CompileFunc(expr, env, imports, mod);
  if (!IsOk(result)) return result;

  MakeLambda(bytes, result);
  return result;
}

static Result CompileCall(Node *expr, u8 **bytes, Frame *env, HashMap *imports, Module *mod)
{
  u32 i;
  Result result;
  u32 num_items = NodeCount(expr);
  Node *op = NodeChild(expr, 0);

  GetReg(bytes, RegEnv);
  for (i = 0; i < num_items - 1; i++) {
    result = CompileExpr(NodeChild(expr, num_items - 1 - i), bytes, env, imports, mod);
    if (!IsOk(result)) return result;
  }

  if (op->type == AccessNode) {
    Node *obj = NodeChild(op, 0);
    Node *name = NodeChild(op, 1);
    if (obj->type == IDNode && name->type == SymbolNode && HashMapContains(imports, NodeValue(obj))) {
      u32 import = ImportIdx(SymbolName(NodeValue(obj)), SymbolName(NodeValue(name)), mod);
      PushByte(bytes, CallOp);
      PushInt(bytes, import);
      SetReg(bytes, RegTmp1);
      SetReg(bytes, RegEnv);
      GetReg(bytes, RegTmp1);
      return Ok(0);
    }
  }

  SetReg(bytes, RegTmp1);
  GetReg(bytes, RegTmp1);
  Head(bytes);
  SetReg(bytes, RegEnv);
  GetReg(bytes, RegTmp1);
  Tail(bytes);
  Call(bytes, AddType(num_items-1, 1, mod));
  SetReg(bytes, RegTmp1);
  SetReg(bytes, RegEnv);
  GetReg(bytes, RegTmp1);

  return Ok(0);
}

static Result CompileDo(Node *expr, u8 **bytes, Frame *env, HashMap *imports, Module *mod)
{
  Result result;
  u32 i;
  u32 num_items = NodeCount(expr);

  for (i = 0; i < num_items; i++) {
    GetReg(bytes, RegEnv);
    result = CompileExpr(NodeChild(expr, i), bytes, env, imports, mod);
    if (!IsOk(result)) return result;

    if (i < num_items - 1) {
      PushByte(bytes, DropOp);
    } else {
      Swap(bytes);
    }
    SetReg(bytes, RegEnv);
  }

  return Ok(0);
}

static Result CompileExpr(Node *expr, u8 **bytes, Frame *env, HashMap *imports, Module *mod)
{
  printf("Compiling %s (%d)\n", NodeName(expr), expr->pos);

  switch (expr->type) {
  case IDNode:        return CompileVar(expr, bytes, env, imports, mod);
  case IntNode:       return CompileInt(expr, bytes, env, imports, mod);
  case AddNode:
  case SubNode:
  case MulNode:
  case DivNode:
  case RemNode:       return CompileBinOp(expr, bytes, env, imports, mod);
  case NotNode:       return CompileNot(expr, bytes, env, imports, mod);
  case NegNode:       return CompileNeg(expr, bytes, env, imports, mod);
  case PairNode:      return CompilePair(expr, bytes, env, imports, mod);
  case ListNode:      return CompileList(expr, bytes, env, imports, mod);
  case TupleNode:     return CompileTuple(expr, bytes, env, imports, mod);
  case LambdaNode:    return CompileLambda(expr, bytes, env, imports, mod);
  case CallNode:      return CompileCall(expr, bytes, env, imports, mod);
  case DoNode:        return CompileDo(expr, bytes, env, imports, mod);
  default:            return CompileError("Unknown expr type", mod->filename, NodePos(expr));
  }
}

static Result CompileFunc(Node *lambda, Frame *env, HashMap *imports, Module *mod)
{
  u32 i;
  Result result;
  Node *params = NodeChild(lambda, 0);
  u32 num_params = NodeCount(params);
  u32 type = AddType(num_params, 1, mod);
  Node *body = NodeChild(lambda, 1);
  Func *func = AddFunc(type, 0, mod);
  u8 **bytes = &func->code;
  env = ExtendFrame(env, num_params);

  for (i = 0; i < num_params; i++) {
    Node *param = NodeChild(params, i);
    FrameSet(env, i, NodeValue(param));
  }

  ExtendEnv(bytes, num_params);
  GetReg(bytes, RegEnv);
  Load(bytes, 0);
  SetReg(bytes, RegTmp1);
  for (i = 0; i < num_params; i++) {
    GetReg(bytes, RegTmp1);
    GetLocal(bytes, i);
    Store(bytes, i*4);
  }

  result = CompileExpr(body, bytes, env, imports, mod);

  PopFrame(env);
  if (!IsOk(result)) return result;

  PushByte(&func->code, EndOp);

  return Ok(FuncIdx(func, mod));
}

#define PageSize 65536
static void CompileUtilities(Module *mod)
{
  u8 **code;
  Func *alloc = AddFunc(AddType(1, 0, mod), 0, mod);
  Func *lookup = AddFunc(AddType(1, 1, mod), 2, mod);
  Func *makepair = AddFunc(AddType(2, 1, mod), 0, mod);
  Func *maketuple = AddFunc(AddType(1, 1, mod), 0, mod);

  code = &alloc->code;
  GetReg(code, RegFree);
  GetLocal(code, 0);
  PushByte(code, I32Add);
  SetReg(code, RegFree);
  PushByte(code, Block);
  PushByte(code, EmptyBlock);
  GetReg(code, RegFree);
  PushByte(code, MemSize);
  PushByte(code, 0);
  Const(code, PageSize);
  PushByte(code, I32Mul);
  PushByte(code, I32LtS);
  PushByte(code, BrIf);
  PushInt(code, 0);
  Const(code, 1);
  PushByte(code, MemGrow);
  PushByte(code, 0);
  PushByte(code, DropOp);
  PushByte(code, EndOp);
  PushByte(code, EndOp);
  AllocFn = FuncIdx(alloc, mod);

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
  Const(code, 0);
  PushByte(code, I32Ne);
  PushByte(code, BrIf);
  PushByte(code, 0);
  PushByte(code, Unreachable);
  PushByte(code, EndOp);
  GetLocal(code, 0);
  GetLocal(code, 1);
  Load(code, 0);
  Const(code, 4);
  PushByte(code, I32Sub);
  Load(code, 0);
  TeeLocal(code, 2);
  PushByte(code, I32LtS);
  PushByte(code, BrIf);
  PushInt(code, 1);
  GetLocal(code, 0);
  GetLocal(code, 2);
  PushByte(code, I32Sub);
  SetLocal(code, 0);
  GetLocal(code, 1);
  Load(code, 4);
  SetLocal(code, 1);
  PushByte(code, Br);
  PushInt(code, 0);
  PushByte(code, EndOp);
  PushByte(code, EndOp);
  GetLocal(code, 1);
  Load(code, 0);
  GetLocal(code, 0);
  Const(code, 4);
  PushByte(code, I32Mul);
  PushByte(code, I32Add);
  Load(code, 0);
  PushByte(code, EndOp);
  LookupFn = FuncIdx(lookup, mod);

  code = &makepair->code;
  GetReg(code, RegFree);
  GetReg(code, RegFree);
  GetReg(code, RegFree);
  Alloc(code, 8);
  GetLocal(code, 1);
  Store(code, 0);
  GetLocal(code, 0);
  Store(code, 4);
  PushByte(code, EndOp);
  MakePairFn = FuncIdx(makepair, mod);

  code = &maketuple->code;
  /* store count */
  GetReg(code, RegFree);
  PushByte(code, LocalGet);
  PushInt(code, 0);
  Alloc(code, 4);
  Store(code, 0);
  /* tuple pointer to return */
  GetReg(code, RegFree);
  /* allocate space for items */
  GetLocal(code, 0);
  Const(code, 4);
  PushByte(code, I32Mul);
  PushByte(code, CallOp);
  PushInt(code, AllocFn);
  PushByte(code, EndOp);
  MakeTupleFn = FuncIdx(maketuple, mod);
}

static void ScanImports(Node *node, HashMap *imports, Module *mod)
{
  u32 i;
  if (IsTerminal(node)) return;

  if (node->type == ImportNode) {
    u32 mod = NodeValue(NodeChild(node, 0));
    u32 alias = NodeValue(NodeChild(node, 1));
    HashMapSet(imports, alias, mod);
    return;
  }

  if (node->type == CallNode) {
    Node *op = NodeChild(node, 0);
    if (op->type == AccessNode) {
      Node *modname = NodeChild(op, 0);
      Node *name = NodeChild(op, 1);
      if (modname->type == IDNode && name->type == SymbolNode) {
        if (HashMapContains(imports, NodeValue(modname))) {
          u32 alias = HashMapGet(imports, NodeValue(modname));
          u32 type = AddType(NodeCount(node)-1, 1, mod);
          AddImport(SymbolName(alias), SymbolName(NodeValue(name)), type, mod);
          return;
        }
      }
    }
  }

  for (i = 0; i < NodeCount(node); i++) {
    ScanImports(NodeChild(node, i), imports, mod);
  }
}

Result CompileModule(Node *ast)
{
  Module *mod = malloc(sizeof(Module));
  u32 i, j;
  Node *stmts;
  Frame *env = 0;
  Result result;
  u32 num_exports = 0;
  Func *start;
  Node *exports;
  HashMap imports = EmptyHashMap;

  if (ast->type != ModuleNode) return CompileError("Not a module", 0, 0);
  exports = ModuleExports(ast);
  num_exports = NodeCount(exports);
  stmts = NodeChild(ModuleBody(ast), 1);

  InitModule(mod);
  mod->num_globals = 5; /* env, free, tmp1, tmp2, tmp3 */
  mod->filename = SymbolName(ModuleFile(ast));

  ScanImports(ast, &imports, mod);

  CompileUtilities(mod);

  start = AddFunc(AddType(0, 0, mod), 0, mod);
  env = ExtendFrame(env, VecCount(mod->imports));
  ExtendEnv(&start->code, VecCount(mod->imports));

  /* pre-define each func */
  if (num_exports > 0) {
    env = ExtendFrame(env, num_exports);
    ExtendEnv(&start->code, num_exports);
    for (i = 0; i < NodeCount(stmts); i++) {
      Node *stmt = NodeChild(stmts, i);
      if (stmt->type != DefNode) continue;
      FrameSet(env, i, NodeValue(NodeChild(stmt, 0)));
    }

    PrintEnv(env);

    /* compile each func */
    for (i = 0; i < NodeCount(stmts); i++) {
      Node *stmt = NodeChild(stmts, i);
      Node *lambda;
      char *name;

      if (stmt->type != DefNode) continue;

      lambda = NodeChild(stmt, 1);
      result = CompileFunc(lambda, env, &imports, mod);
      if (!IsOk(result)) return result;

      GetReg(&start->code, RegEnv);
      Load(&start->code, 0);
      MakeLambda(&start->code, result);
      Store(&start->code, i*4);

      name = SymbolName(NodeValue(NodeChild(stmt, 0)));
      AddExport(name, result, mod);
    }
  }

  /* compile start func */
  for (i = 0, j = 0; i < NodeCount(stmts); i++) {
    Node *stmt = NodeChild(stmts, i);

    if (stmt->type == DefNode) continue;

    result = CompileExpr(stmt, &start->code, env, &imports, mod);
    if (!IsOk(result)) return result;

    if (i == NodeCount(stmts)-1) {
      PushByte(&start->code, CallOp);
      PushInt(&start->code, 0);
    }
    PushByte(&start->code, DropOp);
  }
  PushByte(&start->code, EndOp);

  mod->start = FuncIdx(start, mod);

  return Ok(mod);
}
