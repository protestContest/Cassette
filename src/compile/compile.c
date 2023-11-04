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

static Result CompileExpr(Val node, Val linkage, Compiler *c);
static Result CompileDo(Val stmts, Val linkage, Compiler *c);
static Result CompileAssigns(Val assigns, u32 prev_assigned, Val linkage, Compiler *c);
static Result CompileImport(Val name, u32 prev_assigned, Val linkage, Compiler *c);
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
static u32 ScanAssigns(Val stmts, Compiler *c);
static Result CompileError(char *message, Compiler *c);
#define CompileOk()  OkResult(Nil)

void InitCompiler(Compiler *c, Mem *mem, SymbolTable *symbols, HashMap *modules, Chunk *chunk)
{
  c->mem = mem;
  c->symbols = symbols;
  c->modules = modules;
  c->chunk = chunk;
}

Result CompileModule(Val module, char *filename, Val env, u32 mod_num, Compiler *c)
{
  Val ast = ModuleBody(module, c->mem);
  Val stmts = TupleGet(ast, 2, c->mem);
  u32 num_assigns = ScanAssigns(stmts, c);
  u32 assigned = 0;
  u32 patch;

  c->filename = filename;
  c->pos = 0;
  c->env = ExtendEnv(env, MakeTuple(num_assigns, c->mem), c->mem);

  /* a module is a lambda */
  patch = PushByte(OpLambda, c->pos, c->chunk);
  PushByte(0, 0, c->chunk);
  /* discard arity from apply */
  PushByte(OpPop, 0, c->chunk);

  /* extend env for local assigns */
  PushByte(OpConst, c->pos, c->chunk);
  PushConst(IntVal(num_assigns), c->pos, c->chunk);
  PushByte(OpTuple, c->pos, c->chunk);
  PushByte(OpExtend, c->pos, c->chunk);

  while (stmts != Nil) {
    Val stmt = Head(stmts, c->mem);
    Val tag = TupleGet(stmt, 0, c->mem);
    Val expr = TupleGet(stmt, 2, c->mem);

    if (tag == SymLet) {
      Result result = CompileAssigns(expr, assigned, LinkNext, c);
      if (!result.ok) return result;
      assigned += ListLength(expr, c->mem);
    } else if (tag == SymImport) {
      Val mod;
      Result result = CompileImport(expr, assigned, LinkNext, c);
      if (!result.ok) return result;
      mod = HashMapGet(c->modules, Head(expr, c->mem));
      assigned += ListLength(ModuleExports(mod, c->mem), c->mem);
    } else {
      Result result = CompileExpr(stmt, LinkNext, c);
      if (!result.ok) return result;
    }

    /* discard each statment result */
    PushByte(OpPop, c->pos, c->chunk);

    stmts = Tail(stmts, c->mem);
  }

  /* put two copies of the env on the stack */
  PushByte(OpExport, c->pos, c->chunk);
  PushByte(OpDup, c->pos, c->chunk);

  /* get back to the module scope */
  PushByte(OpPopEnv, c->pos, c->chunk);

  /* redefine the module to the env result */
  PushByte(OpDefine, c->pos, c->chunk);
  PushConst(IntVal(mod_num), c->pos, c->chunk);

  /* the other env copy is returned */
  PushByte(OpReturn, c->pos, c->chunk);
  PatchJump(c->chunk, patch);

  /* define the module in the module scope */
  PushByte(OpDefine, c->pos, c->chunk);
  PushConst(IntVal(mod_num), c->pos, c->chunk);

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
  /*case SymDot:          return CompileOp(OpGet, expr, linkage, c);*/
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

static Result CompileDo(Val stmts, Val linkage, Compiler *c)
{
  u32 num_assigns = ScanAssigns(stmts, c);
  u32 assigned = 0;

  c->env = ExtendEnv(c->env, MakeTuple(num_assigns, c->mem), c->mem);
  PushByte(OpConst, c->pos, c->chunk);
  PushConst(IntVal(num_assigns), 0, c->chunk);
  PushByte(OpTuple, c->pos, c->chunk);
  PushByte(OpExtend, c->pos, c->chunk);

  while (stmts != Nil) {
    Val stmt = Head(stmts, c->mem);
    bool is_last = Tail(stmts, c->mem) == Nil;
    Val stmt_linkage = is_last ? linkage : LinkNext;
    Val tag = TupleGet(stmt, 0, c->mem);
    Val expr = TupleGet(stmt, 2, c->mem);

    if (tag == SymLet) {
      Result result = CompileAssigns(expr, assigned, stmt_linkage, c);
      if (!result.ok) return result;
      assigned += ListLength(expr, c->mem);
    } else if (tag == SymImport) {
      Val mod;
      Result result = CompileImport(expr, assigned, stmt_linkage, c);
      if (!result.ok) return result;
      mod = HashMapGet(c->modules, Head(expr, c->mem));
      assigned += ListLength(ModuleExports(mod, c->mem), c->mem);
    } else {
      Result result = CompileExpr(stmt, stmt_linkage, c);
      if (!result.ok) return result;
    }

    if (!is_last) {
      /* discard each statement result except the last */
      PushByte(OpPop, c->pos, c->chunk);
    }

    stmts = Tail(stmts, c->mem);
  }

  /* cast off our accumulated scopes */
  c->env = Tail(c->env, c->mem);
  if (linkage != LinkReturn) {
    PushByte(OpPopEnv, c->pos, c->chunk);
  }

  return CompileOk();
}

static Result CompileAssigns(Val assigns, u32 prev_assigned, Val linkage, Compiler *c)
{
  u32 i = 0;

  while (assigns != Nil) {
    Val assign = Head(assigns, c->mem);
    Val var = Head(assign, c->mem);
    Val value = Tail(assign, c->mem);
    Val val_linkage = (Tail(assigns, c->mem) == Nil) ? linkage : LinkNext;
    Result result;
    Define(var, prev_assigned + i, c->env, c->mem);

    result = CompileExpr(value, val_linkage, c);
    if (!result.ok) return result;

    PushByte(OpDefine, c->pos, c->chunk);
    PushConst(IntVal(prev_assigned + i), c->pos, c->chunk);

    i++;
    assigns = Tail(assigns, c->mem);
  }

  PushByte(OpConst, c->pos, c->chunk);
  PushConst(Ok, c->pos, c->chunk);

  return CompileOk();
}

static Result CompileImport(Val import, u32 prev_assigned, Val linkage, Compiler *c)
{
  Val mod_name = Head(import, c->mem);
  Val env = c->env;
  u32 frame_num = 0, pos, patch, i;
  Val frame;
  Val module, exports;

  if (!HashMapContains(c->modules, mod_name)) {
    return CompileError("Undefined module", c);
  }
  module = HashMapGet(c->modules, mod_name);
  exports = ModuleExports(module, c->mem);

  /* look for module in second-to-top frame (just below primitives) */
  while (Nil != Tail(Tail(env, c->mem), c->mem)) {
    env = Tail(env, c->mem);
    frame_num++;
  }

  frame = Head(env, c->mem);
  for (pos = 0; pos < TupleLength(frame, c->mem); pos++) {
    if (TupleGet(frame, pos, c->mem) == mod_name) {
      break;
    }
  }

  if (pos == TupleLength(frame, c->mem)) {
    return CompileError("Undefined module", c);
  }

  /* call module */
  patch = PushByte(OpLink, c->pos, c->chunk);
  PushByte(0, c->pos, c->chunk);

  PushByte(OpLookup, c->pos, c->chunk);
  PushConst(IntVal(frame_num), c->pos, c->chunk);
  PushConst(IntVal(pos), c->pos, c->chunk);

  PushByte(OpApply, c->pos, c->chunk);
  PushConst(IntVal(0), c->pos, c->chunk);

  PatchJump(c->chunk, patch);

  i = 0;
  while (exports != Nil) {
    Val export = Head(exports, c->mem);
    Define(export, prev_assigned + i, c->env, c->mem);
    PushByte(OpGet, c->pos, c->chunk);
    PushConst(IntVal(i), c->pos, c->chunk);
    PushByte(OpDefine, c->pos, c->chunk);
    PushConst(IntVal(prev_assigned + i), c->pos, c->chunk);

    i++;
    exports = Tail(exports, c->mem);
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
    PushByte(OpNoop, c->pos, c->chunk);
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
  u32 patch, i;
  u32 num_params = ListLength(params, c->mem);

  patch = PushByte(OpLambda, c->pos, c->chunk);
  PushByte(0, c->pos, c->chunk);

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
  }

  result = CompileExpr(body, LinkReturn, c);
  if (!result.ok) return result;

  PatchJump(c->chunk, patch);

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

static u32 ScanAssigns(Val stmts, Compiler *c)
{
  u32 assigns = 0;
  while (stmts != Nil) {
    Val stmt = Head(stmts, c->mem);
    Val type = TupleGet(stmt, 0, c->mem);
    Val expr = TupleGet(stmt, 2, c->mem);

    if (type == SymLet) {
      assigns += ListLength(expr, c->mem);
    } else if (type == SymImport) {
      Val mod_name = Head(expr, c->mem);
      if (HashMapContains(c->modules, mod_name)) {
        Val mod = HashMapGet(c->modules, mod_name);
        assigns += ListLength(ModuleExports(mod, c->mem), c->mem);
      }
    }

    stmts = Tail(stmts, c->mem);
  }
  return assigns;
}

static Result CompileError(char *message, Compiler *c)
{
  return ErrorResult(message, c->filename, c->pos);
}
