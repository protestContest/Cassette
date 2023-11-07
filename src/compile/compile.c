#include "compile.h"
#include "parse.h"
#include "runtime/env.h"
#include "runtime/ops.h"
#include "module.h"
#include "source.h"
#include <stdio.h>
#include <stdlib.h>

#define LinkReturn  0x7FD336A7
#define LinkNext    0x7FD9CB70

static Result CompileImports(Val imports, Compiler *c);
static Result CompileExpr(Val node, Val linkage, Compiler *c);
static Result CompileBlock(Val stmts, Val assigns, Val linkage, Compiler *c);
static Result CompileDo(Val expr, Val linkage, Compiler *c);
static Result CompileCall(Val call, Val linkage, Compiler *c);
static Result CompileLambda(Val expr, Val linkage, Compiler *c);
static Result CompileIf(Val expr, Val linkage, Compiler *c);
static Result CompileAnd(Val expr, Val linkage, Compiler *c);
static Result CompileOr(Val expr, Val linkage, Compiler *c);
static Result CompileOp(OpCode op, Val expr, Val linkage, Compiler *c);
static Result CompileNotOp(OpCode op, Val expr, Val linkage, Compiler *c);
static Result CompileList(Val items, Val linkage, Compiler *c);
static Result CompileTuple(Val items, Val linkage, Compiler *c);
static Result CompileString(Val sym, Val linkage, Compiler *c);
static Result CompileVar(Val id, Val linkage, Compiler *c);
static Result CompileConst(Val value, Val linkage, Compiler *c);
static Result CompileGroup(Val expr, Val linkage, Compiler *c);

static void CompileLinkage(Val linkage, Compiler *c);
static Result CompileError(char *message, Compiler *c);
#define CompileOk()  OkResult(Nil)

void InitCompiler(Compiler *c, Mem *mem, SymbolTable *symbols, HashMap *modules, Chunk *chunk)
{
  c->pos = 0;
  c->env = Nil;
  c->filename = 0;
  c->mem = mem;
  c->symbols = symbols;
  c->modules = modules;
  c->chunk = chunk;
}

Result CompileScript(Val module, Compiler *c)
{
  Result result;
  Val imports = ModuleImports(module, c->mem);

  c->filename = SymbolName(ModuleFile(module, c->mem), c->symbols);
  c->pos = 0;

  /* copy filename symbol to chunk */
  Sym(c->filename, &c->chunk->symbols);

  /* record start position of module in chunk */
  BeginChunkFile(ModuleFile(module, c->mem), c->chunk);

  /* compile imports */
  if (imports != Nil) {
    u32 num_imports = CountExports(imports, c->modules, c->mem);

    PushByte(OpConst, c->pos, c->chunk);
    PushConst(IntVal(num_imports), c->pos, c->chunk);
    PushByte(OpTuple, c->pos, c->chunk);
    PushByte(OpExtend, c->pos, c->chunk);
    c->env = ExtendEnv(c->env, MakeTuple(num_imports, c->mem), c->mem);

    result = CompileImports(imports, c);
    if (!result.ok) return result;
  }

  result = CompileDo(Pair(ModuleExports(module, c->mem), ModuleBody(module, c->mem), c->mem), LinkNext, c);
  if (!result.ok) return result;

  /* discard import frame */
  if (imports != Nil) {
    PushByte(OpExport, c->pos, c->chunk);
    PushByte(OpPop, c->pos, c->chunk);
    c->env = Tail(c->env, c->mem);
  }

  EndChunkFile(c->chunk);

  return CompileOk();
}

Result CompileModule(Val module, u32 mod_num, Compiler *c)
{
  Result result;
  Val imports = ModuleImports(module, c->mem);
  u32 num_assigns = ListLength(ModuleExports(module, c->mem), c->mem);
  u32 jump;

  c->filename = SymbolName(ModuleFile(module, c->mem), c->symbols);
  c->pos = 0;

  /* copy filename symbol to chunk */
  Sym(c->filename, &c->chunk->symbols);

  /* record start position of module in chunk */
  BeginChunkFile(ModuleFile(module, c->mem), c->chunk);

  /* create lambda for module */
  PushByte(OpLink, c->pos, c->chunk);
  PushConst(IntVal(5), c->pos, c->chunk);
  PushByte(OpPair, c->pos, c->chunk);
  jump = PushByte(OpJump, c->pos, c->chunk);
  PushByte(0, c->pos, c->chunk);

  PushByte(OpPop, c->pos, c->chunk); /* discard arity from apply */

  /* compile imports */
  if (imports != Nil) {
    u32 num_imports = CountExports(imports, c->modules, c->mem);

    PushByte(OpConst, c->pos, c->chunk);
    PushConst(IntVal(num_imports), c->pos, c->chunk);
    PushByte(OpTuple, c->pos, c->chunk);
    PushByte(OpExtend, c->pos, c->chunk);
    c->env = ExtendEnv(c->env, MakeTuple(num_imports, c->mem), c->mem);

    result = CompileImports(imports, c);
    if (!result.ok) return result;
  }

  /* extend env for assigns */
  if (num_assigns > 0) {
    c->env = ExtendEnv(c->env, MakeTuple(num_assigns, c->mem), c->mem);
    PushByte(OpConst, c->pos, c->chunk);
    PushConst(IntVal(num_assigns), c->pos, c->chunk);
    PushByte(OpTuple, c->pos, c->chunk);
    PushByte(OpExtend, c->pos, c->chunk);
  }

  result = CompileBlock(ModuleBody(module, c->mem), ModuleExports(module, c->mem), LinkNext, c);
  if (!result.ok) return result;

  /* discard last statement result */
  PushByte(OpPop, c->pos, c->chunk);

  /* export assigned values */
  if (num_assigns > 0) {
    PushByte(OpExport, c->pos, c->chunk);
    c->env = Tail(c->env, c->mem);
  } else {
    PushByte(OpConst, c->pos, c->chunk);
    PushConst(IntVal(0), c->pos, c->chunk);
    PushByte(OpTuple, c->pos, c->chunk);
  }
  PushByte(OpDup, c->pos, c->chunk);

  /* discard import frame */
  if (imports != Nil) {
    PushByte(OpExport, c->pos, c->chunk);
    PushByte(OpPop, c->pos, c->chunk);
    c->env = Tail(c->env, c->mem);
  }

  /* redefine module as exported values */
  PushByte(OpDefine, c->pos, c->chunk);
  PushConst(IntVal(mod_num), c->pos, c->chunk);

  PushByte(OpReturn, c->pos, c->chunk);

  PatchJump(c->chunk, jump);
  PushByte(OpDefine, c->pos, c->chunk);
  PushConst(IntVal(mod_num), c->pos, c->chunk);

  EndChunkFile(c->chunk);

  return CompileOk();
}

static Result CompileImports(Val imports, Compiler *c)
{
  u32 num_imported = 0;

  while (imports != Nil) {
    Val node = Head(imports, c->mem);
    Val import_name = NodeExpr(node, c->mem);
    Val import_mod;
    Val imported_vals;
    u32 i;
    i32 import_def;

    c->pos = NodePos(node, c->mem);

    if (!HashMapContains(c->modules, import_name)) return CompileError("Undefined module", c);
    import_mod = HashMapGet(c->modules, import_name);
    imported_vals = ModuleExports(import_mod, c->mem);

    import_def = FindDefinition(import_name, c->env, c->mem);
    if (import_def < 0) return CompileError("Undefined module", c);

    /* call module function */
    PushByte(OpLink, c->pos, c->chunk);
    PushConst(IntVal(7), c->pos, c->chunk);
    PushByte(OpLookup, c->pos, c->chunk);
    PushConst(IntVal(import_def >> 16), c->pos, c->chunk);
    PushConst(IntVal(import_def & 0xFFFF), c->pos, c->chunk);
    PushByte(OpApply, c->pos, c->chunk);
    PushConst(IntVal(0), c->pos, c->chunk);

    /* define each exported value */
    i = 0;
    while (imported_vals != Nil) {
      Val imported_val = Head(imported_vals, c->mem);

      PushByte(OpGet, c->pos, c->chunk);
      PushConst(IntVal(i), c->pos, c->chunk);
      PushByte(OpDefine, c->pos, c->chunk);
      PushConst(IntVal(num_imported+i), c->pos, c->chunk);
      Define(imported_val, num_imported + i, c->env, c->mem);

      imported_vals = Tail(imported_vals, c->mem);
      i++;
    }
    num_imported += i;

    /* discard imported frame */
    PushByte(OpPop, c->pos, c->chunk);

    imports = Tail(imports, c->mem);
  }

  return CompileOk();
}

static Result CompileExpr(Val node, Val linkage, Compiler *c)
{
  Val tag = TupleGet(node, 0, c->mem);
  Val expr = TupleGet(node, 2, c->mem);
  c->pos = RawInt(TupleGet(node, 1, c->mem));

  switch (tag) {
  case SymID:           return CompileVar(expr, linkage, c);
  case SymBangEqual:    return CompileNotOp(OpEq, expr, linkage, c);
  case SymString:       return CompileString(expr, linkage, c);
  case SymHash:         return CompileOp(OpLen, expr, linkage, c);
  case SymPercent:      return CompileOp(OpRem, expr, linkage, c);
  case SymLParen:       return CompileCall(expr, linkage, c);
  case SymStar:         return CompileOp(OpMul, expr, linkage, c);
  case SymPlus:         return CompileOp(OpAdd, expr, linkage, c);
  case SymMinus:
    if (ListLength(expr, c->mem) == 1) return CompileOp(OpNeg, expr, linkage, c);
    else return CompileOp(OpSub, expr, linkage, c);
  case SymArrow:        return CompileLambda(expr, linkage, c);
  case SymSlash:        return CompileOp(OpDiv, expr, linkage, c);
  case SymNum:          return CompileConst(expr, linkage, c);
  case SymColon:        return CompileConst(expr, linkage, c);
  case SymLess:         return CompileOp(OpLt, expr, linkage, c);
  case SymLessEqual:    return CompileNotOp(OpGt, expr, linkage, c);
  case SymEqualEqual:   return CompileOp(OpEq, expr, linkage, c);
  case SymGreater:      return CompileOp(OpGt, expr, linkage, c);
  case SymGreaterEqual: return CompileNotOp(OpLt, expr, linkage, c);
  case SymLBracket:     return CompileList(expr, linkage, c);
  case SymAnd:          return CompileAnd(expr, linkage, c);
  case SymDo:           return CompileDo(expr, linkage, c);
  case SymIf:           return CompileIf(expr, linkage, c);
  case SymIn:           return CompileOp(OpIn, expr, linkage, c);
  case SymNil:          return CompileConst(Nil, linkage, c);
  case SymNot:          return CompileOp(OpNot, expr, linkage, c);
  case SymOr:           return CompileOr(expr, linkage, c);
  case SymLBrace:       return CompileTuple(expr, linkage, c);
  case SymBar:          return CompileOp(OpPair, expr, linkage, c);
  default:              return CompileError("Unknown expression", c);
  }
}

static Result CompileBlock(Val stmts, Val assigns, Val linkage, Compiler *c)
{
  u32 assigned = 0;

  while (stmts != Nil) {
    Result result;
    Val stmt = Head(stmts, c->mem);
    bool is_last = Tail(stmts, c->mem) == Nil;
    Val stmt_linkage = is_last ? linkage : LinkNext;

    if (NodeType(stmt, c->mem) == SymLet) {
      Val var = Head(NodeExpr(stmt, c->mem), c->mem);
      Val value = Tail(NodeExpr(stmt, c->mem), c->mem);

      Define(var, assigned, c->env, c->mem);

      result = CompileExpr(value, stmt_linkage, c);
      if (!result.ok) return result;

      PushByte(OpDefine, c->pos, c->chunk);
      PushConst(IntVal(assigned), c->pos, c->chunk);

      assigned++;

      if (is_last) {
        PushByte(OpConst, c->pos, c->chunk);
        PushConst(Ok, c->pos, c->chunk);
      }
    } else {
      result = CompileExpr(stmt, stmt_linkage, c);
      if (!result.ok) return result;
      if (!is_last) {
        /* discard each statement result except the last */
        PushByte(OpPop, c->pos, c->chunk);
      }
    }

    stmts = Tail(stmts, c->mem);
  }

  return CompileOk();
}

static Result CompileDo(Val expr, Val linkage, Compiler *c)
{
  Result result;
  Val assigns = Head(expr, c->mem);
  Val stmts = Tail(expr, c->mem);

  u32 num_assigns = ListLength(assigns, c->mem);

  if (num_assigns > 0) {
    c->env = ExtendEnv(c->env, MakeTuple(num_assigns, c->mem), c->mem);
    PushByte(OpConst, c->pos, c->chunk);
    PushConst(IntVal(num_assigns), 0, c->chunk);
    PushByte(OpTuple, c->pos, c->chunk);
    PushByte(OpExtend, c->pos, c->chunk);
  }

  result = CompileBlock(stmts, assigns, linkage, c);
  if (!result.ok) return result;

  /* discard frame for assigns */
  if (num_assigns > 0) {
    PushByte(OpExport, c->pos, c->chunk);
    PushByte(OpPop, c->pos, c->chunk);
    c->env = Tail(c->env, c->mem);
  }

  return CompileOk();
}

static Result CompileCall(Val call, Val linkage, Compiler *c)
{
  Val op = Head(call, c->mem);
  Val args = Tail(call, c->mem);
  u32 num_args = 0, patch;
  Result result;

  if (linkage != LinkReturn) {
    patch = PushByte(OpLink, c->pos, c->chunk);
    PushByte(0, c->pos, c->chunk);
  }

  while (args != Nil) {
    Val arg = Head(args, c->mem);

    result = CompileExpr(arg, LinkNext, c);
    if (!result.ok) return result;
    args = Tail(args, c->mem);
    num_args++;
  }

  result = CompileExpr(op, LinkNext, c);
  if (!result.ok) return result;

  PushByte(OpApply, c->pos, c->chunk);
  PushConst(IntVal(num_args), c->pos, c->chunk);

  if (linkage == LinkNext) {
    PatchJump(c->chunk, patch);
  } else if (linkage != LinkReturn) {
    PatchChunk(c->chunk, patch+1, linkage);
  }

  return CompileOk();
}

static Result CompileLambda(Val expr, Val linkage, Compiler *c)
{
  Val params = Head(expr, c->mem);
  Val body = Tail(expr, c->mem);
  Result result;
  u32 jump, branch, i;
  u32 num_params = ListLength(params, c->mem);

  /* create lambda */
  PushByte(OpLink, c->pos, c->chunk);
  PushConst(IntVal(5), c->pos, c->chunk);
  PushByte(OpPair, c->pos, c->chunk);
  jump = PushByte(OpJump, c->pos, c->chunk);
  PushByte(0, c->pos, c->chunk);

  /* arity check */
  PushByte(OpDup, c->pos, c->chunk);
  PushByte(OpConst, c->pos, c->chunk);
  PushConst(IntVal(num_params), c->pos, c->chunk);
  PushByte(OpEq, c->pos, c->chunk);
  branch = PushByte(OpBranch, c->pos, c->chunk);
  PushByte(0, c->pos, c->chunk);
  PushByte(OpPop, c->pos, c->chunk);
  PushByte(OpConst, c->pos, c->chunk);
  PushConst(Error, c->pos, c->chunk);
  PushByte(OpError, c->pos, c->chunk);
  PatchJump(c->chunk, branch);
  PushByte(OpPop, c->pos, c->chunk);

  if (num_params > 0) {
    PushByte(OpTuple, c->pos, c->chunk);
    PushByte(OpExtend, c->pos, c->chunk);

    c->env = ExtendEnv(c->env, MakeTuple(num_params, c->mem), c->mem);
    for (i = 0; i < num_params; i++) {
      Val param = Head(params, c->mem);
      Define(param, i, c->env, c->mem);
      PushByte(OpDefine, c->pos, c->chunk);
      PushConst(IntVal(num_params - i - 1), c->pos, c->chunk);
      params = Tail(params, c->mem);
    }
  } else {
    PushByte(OpPop, c->pos, c->chunk);
  }

  result = CompileExpr(body, LinkReturn, c);
  if (!result.ok) return result;

  PatchJump(c->chunk, jump);

  if (num_params > 0) c->env = Tail(c->env, c->mem);

  return CompileOk();
}

static Result CompileIf(Val expr, Val linkage, Compiler *c)
{
  Val pred = Head(expr, c->mem);
  Val cons = Head(Tail(expr, c->mem), c->mem);
  Val alt = Head(Tail(Tail(expr, c->mem), c->mem), c->mem);
  Result result;
  u32 branch, jump;

  result = CompileExpr(pred, LinkNext, c);
  if (!result.ok) return result;

  branch = PushByte(OpBranch, c->pos, c->chunk);
  PushByte(0, c->pos, c->chunk);

  PushByte(OpPop, c->pos, c->chunk);
  result = CompileExpr(alt, linkage, c);
  if (!result.ok) return result;
  if (linkage == LinkNext) {
    jump = PushByte(OpJump, c->pos, c->chunk);
    PushByte(0, c->pos, c->chunk);
  }

  PatchJump(c->chunk, branch);

  PushByte(OpPop, c->pos, c->chunk);
  result = CompileExpr(cons, linkage, c);
  if (!result.ok) return result;

  if (linkage == LinkNext) {
    PatchJump(c->chunk, jump);
  }

  return CompileOk();
}

static Result CompileAnd(Val expr, Val linkage, Compiler *c)
{
  Val a = Head(expr, c->mem);
  Val b = Head(Tail(expr, c->mem), c->mem);
  u32 branch, jump;

  Result result = CompileExpr(a, LinkNext, c);
  if (!result.ok) return result;

  branch = PushByte(OpBranch, c->pos, c->chunk);
  PushByte(0, c->pos, c->chunk);

  if (linkage == LinkNext) {
    jump = PushByte(OpJump, c->pos, c->chunk);
    PushByte(0, c->pos, c->chunk);
  } else {
    CompileLinkage(linkage, c);
  }

  PatchJump(c->chunk, branch);

  PushByte(OpPop, c->pos, c->chunk);
  result = CompileExpr(b, linkage, c);
  if (!result.ok) return result;

  if (linkage == LinkNext) {
    PatchJump(c->chunk, jump);
  }

  return CompileOk();
}

static Result CompileOr(Val expr, Val linkage, Compiler *c)
{
  Val a = Head(expr, c->mem);
  Val b = Head(Tail(expr, c->mem), c->mem);
  u32 branch;

  Result result = CompileExpr(a, LinkNext, c);
  if (!result.ok) return result;

  branch = PushByte(OpBranch, c->pos, c->chunk);
  PushByte(0, c->pos, c->chunk);

  PushByte(OpPop, c->pos, c->chunk);
  result = CompileExpr(b, linkage, c);
  if (!result.ok) return result;

  if (linkage == LinkNext || linkage == LinkReturn) {
    PatchJump(c->chunk, branch);
    CompileLinkage(linkage, c);
  } else {
    PatchChunk(c->chunk, branch+1, linkage);
  }

  return CompileOk();
}

static Result CompileOp(OpCode op, Val args, Val linkage, Compiler *c)
{
  u32 pos = c->pos;
  while (args != Nil) {
    Result result = CompileExpr(Head(args, c->mem), LinkNext, c);
    if (!result.ok) return result;
    args = Tail(args, c->mem);
  }
  PushByte(op, pos, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileNotOp(OpCode op, Val args, Val linkage, Compiler *c)
{
  while (args != Nil) {
    Result result = CompileExpr(Head(args, c->mem), LinkNext, c);
    if (!result.ok) return result;
    args = Tail(args, c->mem);
  }
  PushByte(op, c->pos, c->chunk);
  PushByte(OpNot, c->pos, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileList(Val items, Val linkage, Compiler *c)
{
  PushByte(OpConst, c->pos, c->chunk);
  PushConst(Nil, c->pos, c->chunk);

  while (items != Nil) {
    Val item = Head(items, c->mem);
    Result result = CompileExpr(item, LinkNext, c);
    if (!result.ok) return result;
    PushByte(OpPair, c->pos, c->chunk);
    items = Tail(items, c->mem);
  }

  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileTuple(Val items, Val linkage, Compiler *c)
{
  u32 i, num_items = 0;

  while (items != Nil) {
    Val item = Head(items, c->mem);
    Result result = CompileExpr(item, LinkNext, c);
    if (!result.ok) return result;
    items = Tail(items, c->mem);
    num_items++;
  }

  PushByte(OpConst, c->pos, c->chunk);
  PushConst(IntVal(num_items), c->pos, c->chunk);
  PushByte(OpTuple, c->pos, c->chunk);
  for (i = 0; i < num_items; i++) {
    PushByte(OpSet, c->pos, c->chunk);
    PushConst(IntVal(i), c->pos, c->chunk);
  }

  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileString(Val sym, Val linkage, Compiler *c)
{
  Sym(SymbolName(sym, c->symbols), &c->chunk->symbols);
  PushByte(OpConst, c->pos, c->chunk);
  PushConst(sym, c->pos, c->chunk);
  PushByte(OpStr, c->pos, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileVar(Val id, Val linkage, Compiler *c)
{
  i32 def = FindDefinition(id, c->env, c->mem);
  if (def == -1) return CompileError("Undefined variable", c);

  PushByte(OpLookup, c->pos, c->chunk);
  PushConst(IntVal(def >> 16), c->pos, c->chunk);
  PushConst(IntVal(def & 0xFFFF), c->pos, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileConst(Val value, Val linkage, Compiler *c)
{
  PushByte(OpConst, c->pos, c->chunk);
  PushConst(value, c->pos, c->chunk);
  CompileLinkage(linkage, c);
  if (IsSym(value)) {
    Sym(SymbolName(value, c->symbols), &c->chunk->symbols);
  }
  return CompileOk();
}

static void CompileLinkage(Val linkage, Compiler *c)
{
  if (linkage == LinkReturn) {
    PushByte(OpReturn, c->pos, c->chunk);
  } else if (linkage != LinkNext) {
    PushConst(linkage, c->pos, c->chunk);
    PushByte(OpJump, c->pos, c->chunk);
  }
}

static Result CompileError(char *message, Compiler *c)
{
  return ErrorResult(message, c->filename, c->pos);
}
