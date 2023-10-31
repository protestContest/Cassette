#include "compile.h"
#include "parse.h"
#include "vm.h"
#include "env.h"
#include <stdio.h>
#include <stdlib.h>

#define LinkReturn  0x7FD336A7
#define LinkNext    0x7FD9CB70

static CompileResult CompileExpr(Val node, Val linkage, Compiler *c);
static CompileResult CompileDo(Val stmts, Val linkage, Compiler *c);
static CompileResult CompileAssigns(Val assigns, Val linkage, Compiler *c);
static CompileResult CompileCall(Val call, Val linkage, Compiler *c);
static CompileResult CompileLambda(Val expr, Val linkage, Compiler *c);
static CompileResult CompileIf(Val expr, Val linkage, Compiler *c);
static CompileResult CompileAnd(Val expr, Val linkage, Compiler *c);
static CompileResult CompileOr(Val expr, Val linkage, Compiler *c);
static CompileResult CompileOp(OpCode op, Val expr, Val linkage, Compiler *c);
static CompileResult CompileNotOp(OpCode op, Val expr, Val linkage, Compiler *c);
static CompileResult CompileList(Val items, Val linkage, Compiler *c);
static CompileResult CompileTuple(Val items, Val linkage, Compiler *c);
static CompileResult CompileString(Val sym, Val linkage, Compiler *c);
static CompileResult CompileVar(Val id, Val linkage, Compiler *c);
static CompileResult CompileConst(Val value, Val linkage, Compiler *c);
static CompileResult CompileGroup(Val expr, Val linkage, Compiler *c);

static void CompileLinkage(Val linkage, Compiler *c);
static CompileResult CompileOk(void);
static CompileResult CompileError(char *message, Compiler *c);

void InitCompiler(Compiler *c, Chunk *chunk, PrimitiveDef *primitives, u32 num_primitives)
{
  c->env = Nil;
  InitMem(&c->mem, 1024);
  MakeParseSyms(&chunk->symbols);
  c->chunk = chunk;

  if (num_primitives > 0) {
    Val frame = MakeTuple(num_primitives, &c->mem);
    u32 i;
    for (i = 0; i < num_primitives; i++) {
      TupleSet(frame, i, Sym(primitives[i].name, &c->chunk->symbols), &c->mem);
    }
    c->env = ExtendEnv(c->env, frame, &c->mem);
  }
}

void DestroyCompiler(Compiler *c)
{
  DestroyMem(&c->mem);
}

CompileResult Compile(char *source, Compiler *c)
{
  Val ast;

  ast = Parse(source, c);
  if (ast == Error) return CompileError("Parse error", c);

  return CompileExpr(ast, LinkNext, c);
}

static CompileResult CompileExpr(Val node, Val linkage, Compiler *c)
{
  Val tag = TupleGet(node, 0, &c->mem);
  Val expr = TupleGet(node, 2, &c->mem);
  c->lex.pos = RawInt(TupleGet(node, 1, &c->mem));

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
    if (ListLength(expr, &c->mem) == 1) return CompileOp(OpNeg, expr, linkage, c);
    else return CompileOp(OpSub, expr, linkage, c);
  case SymArrow:        return CompileLambda(expr, linkage, c);
  case SymDot:          return CompileOp(OpGet, expr, linkage, c);
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

static CompileResult CompileDo(Val stmts, Val linkage, Compiler *c)
{
  u32 scopes = 0, i;

  while (stmts != Nil) {
    Val stmt = Head(stmts, &c->mem);
    bool is_last = Tail(stmts, &c->mem) == Nil;
    Val stmt_linkage = is_last ? linkage : LinkNext;
    Val tag = TupleGet(stmt, 0, &c->mem);
    Val expr = TupleGet(stmt, 2, &c->mem);

    if (tag == SymLet) {
      CompileResult result = CompileAssigns(expr, stmt_linkage, c);
      if (!result.ok) return result;

      scopes++;
      if (is_last) {
        PushByte(OpConst, c->chunk);
        PushConst(Ok, c->chunk);
      }
    } else {
      CompileResult result = CompileExpr(stmt, stmt_linkage, c);
      if (!result.ok) return result;

      if (!is_last) {
        PushByte(OpPop, c->chunk);
      }
    }

    stmts = Tail(stmts, &c->mem);
  }

  /* cast off our accumulated scopes */
  for (i = 0; i < scopes; i++) {
    c->env = Tail(c->env, &c->mem);
  }

  return CompileOk();
}

static CompileResult CompileAssigns(Val assigns, Val linkage, Compiler *c)
{
  u32 num_assigns = ListLength(assigns, &c->mem);
  u32 i = 0;

  c->env = ExtendEnv(c->env, MakeTuple(num_assigns, &c->mem), &c->mem);
  PushByte(OpConst, c->chunk);
  PushConst(IntVal(num_assigns), c->chunk);
  PushByte(OpTuple, c->chunk);
  PushByte(OpExtend, c->chunk);

  while (assigns != Nil) {
    Val assign = Head(assigns, &c->mem);
    Val var = Head(assign, &c->mem);
    Val value = Tail(assign, &c->mem);
    Val val_linkage = (Tail(assigns, &c->mem) == Nil) ? linkage : LinkNext;
    CompileResult result;
    Define(var, i, c->env, &c->mem);

    result = CompileExpr(value, val_linkage, c);
    if (!result.ok) return result;

    PushByte(OpDefine, c->chunk);
    PushConst(IntVal(i), c->chunk);

    i++;
    assigns = Tail(assigns, &c->mem);
  }

  return CompileOk();
}

static CompileResult CompileCall(Val call, Val linkage, Compiler *c)
{
  Val op = Head(call, &c->mem);
  Val args = Tail(call, &c->mem);
  u32 num_args = 0;
  CompileResult result;

  u32 patch = PushByte(OpNoop, c->chunk);
  PushByte(OpNoop, c->chunk);

  while (args != Nil) {
    Val arg = Head(args, &c->mem);

    result = CompileExpr(arg, LinkNext, c);
    if (!result.ok) return result;
    args = Tail(args, &c->mem);
    num_args++;
  }

  result = CompileExpr(op, LinkNext, c);
  if (!result.ok) return result;

  PushByte(OpApply, c->chunk);
  PushConst(IntVal(num_args), c->chunk);

  if (linkage == LinkNext) linkage = IntVal(c->chunk->count - patch);
  if (linkage != LinkReturn) {
    c->chunk->code[patch] = OpLink;
    PatchChunk(c->chunk, patch+1, linkage);
  }

  return CompileOk();
}

static CompileResult CompileLambda(Val expr, Val linkage, Compiler *c)
{
  Val params = Head(expr, &c->mem);
  Val body = Tail(expr, &c->mem);
  CompileResult result;
  u32 patch, i;
  u32 num_params = ListLength(params, &c->mem);

  patch = PushByte(OpLambda, c->chunk);
  PushByte(0, c->chunk);

  if (num_params > 0) {
    PushByte(OpTuple, c->chunk);
    PushByte(OpExtend, c->chunk);

    c->env = ExtendEnv(c->env, MakeTuple(num_params, &c->mem), &c->mem);
    for (i = 0; i < num_params; i++) {
      Val param = Head(params, &c->mem);
      Define(param, i, c->env, &c->mem);
      PushByte(OpDefine, c->chunk);
      PushConst(IntVal(num_params - i - 1), c->chunk);
      params = Tail(params, &c->mem);
    }
  }

  result = CompileExpr(body, LinkReturn, c);
  if (!result.ok) return result;

  PatchChunk(c->chunk, patch+1, IntVal(c->chunk->count - patch));

  if (num_params > 0) c->env = Tail(c->env, &c->mem);

  return CompileOk();
}

static CompileResult CompileIf(Val expr, Val linkage, Compiler *c)
{
  Val pred = Head(expr, &c->mem);
  Val cons = Head(Tail(expr, &c->mem), &c->mem);
  Val alt = Head(Tail(Tail(expr, &c->mem), &c->mem), &c->mem);
  CompileResult result;
  u32 branch, jump;

  result = CompileExpr(pred, LinkNext, c);
  if (!result.ok) return result;

  branch = PushByte(OpBranch, c->chunk);
  PushByte(0, c->chunk);

  PushByte(OpPop, c->chunk);
  result = CompileExpr(alt, linkage, c);
  if (!result.ok) return result;
  if (linkage == LinkNext) {
    jump = PushByte(OpJump, c->chunk);
    PushByte(0, c->chunk);
  }

  PatchChunk(c->chunk, branch+1, IntVal(c->chunk->count - branch));

  PushByte(OpPop, c->chunk);
  result = CompileExpr(cons, linkage, c);

  if (linkage == LinkNext) {
    PatchChunk(c->chunk, jump+1, IntVal(c->chunk->count  - jump));
  }

  return CompileOk();
}

static CompileResult CompileAnd(Val expr, Val linkage, Compiler *c)
{
  Val a = Head(expr, &c->mem);
  Val b = Head(Tail(expr, &c->mem), &c->mem);
  u32 branch, jump;

  CompileResult result = CompileExpr(a, LinkNext, c);
  if (!result.ok) return result;

  branch = PushByte(OpBranch, c->chunk);
  PushByte(0, c->chunk);

  if (linkage == LinkNext) {
    jump = PushByte(OpJump, c->chunk);
    PushByte(0, c->chunk);
  } else {
    CompileLinkage(linkage, c);
  }

  PatchChunk(c->chunk, branch+1, IntVal(c->chunk->count - branch));

  PushByte(OpPop, c->chunk);
  result = CompileExpr(b, linkage, c);
  if (!result.ok) return result;

  if (linkage == LinkNext) {
    PatchChunk(c->chunk, jump+1, IntVal(c->chunk->count - jump));
  }

  return CompileOk();
}

static CompileResult CompileOr(Val expr, Val linkage, Compiler *c)
{
  Val a = Head(expr, &c->mem);
  Val b = Head(Tail(expr, &c->mem), &c->mem);
  u32 branch;

  CompileResult result = CompileExpr(a, LinkNext, c);
  if (!result.ok) return result;

  branch = PushByte(OpBranch, c->chunk);
  PushByte(0, c->chunk);

  PushByte(OpPop, c->chunk);
  result = CompileExpr(b, linkage, c);
  if (!result.ok) return result;

  if (linkage == LinkNext || linkage == LinkReturn) {
    PatchChunk(c->chunk, branch+1, IntVal(c->chunk->count - branch));
    CompileLinkage(linkage, c);
  } else {
    PatchChunk(c->chunk, branch+1, linkage);
  }

  return CompileOk();
}

static CompileResult CompileOp(OpCode op, Val args, Val linkage, Compiler *c)
{
  while (args != Nil) {
    CompileResult result = CompileExpr(Head(args, &c->mem), LinkNext, c);
    if (!result.ok) return result;
    args = Tail(args, &c->mem);
  }
  PushByte(op, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static CompileResult CompileNotOp(OpCode op, Val args, Val linkage, Compiler *c)
{
  while (args != Nil) {
    CompileResult result = CompileExpr(Head(args, &c->mem), LinkNext, c);
    if (!result.ok) return result;
  }
  PushByte(op, c->chunk);
  PushByte(OpNot, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static CompileResult CompileList(Val items, Val linkage, Compiler *c)
{
  PushByte(OpConst, c->chunk);
  PushConst(Nil, c->chunk);

  while (items != Nil) {
    Val item = Head(items, &c->mem);
    CompileResult result = CompileExpr(item, LinkNext, c);
    if (!result.ok) return result;
    PushByte(OpPair, c->chunk);
    items = Tail(items, &c->mem);
  }

  CompileLinkage(linkage, c);
  return CompileOk();
}

static CompileResult CompileTuple(Val items, Val linkage, Compiler *c)
{
  u32 i, num_items = 0;

  while (items != Nil) {
    Val item = Head(items, &c->mem);
    CompileResult result = CompileExpr(item, LinkNext, c);
    if (!result.ok) return result;
    items = Tail(items, &c->mem);
    num_items++;
  }

  PushByte(OpConst, c->chunk);
  PushConst(IntVal(num_items), c->chunk);
  PushByte(OpTuple, c->chunk);
  for (i = 0; i < num_items; i++) {
    PushByte(OpSet, c->chunk);
    PushConst(IntVal(i), c->chunk);
  }

  CompileLinkage(linkage, c);
  return CompileOk();
}

static CompileResult CompileString(Val sym, Val linkage, Compiler *c)
{
  PushByte(OpConst, c->chunk);
  PushConst(sym, c->chunk);
  PushByte(OpStr, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static CompileResult CompileVar(Val id, Val linkage, Compiler *c)
{
  i32 def = FindDefinition(id, c->env, &c->mem);
  if (def == -1) return CompileError("Undefined variable", c);

  PushByte(OpLookup, c->chunk);
  PushConst(IntVal(def >> 16), c->chunk);
  PushConst(IntVal(def & 0xFFFF), c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static CompileResult CompileConst(Val value, Val linkage, Compiler *c)
{
  PushByte(OpConst, c->chunk);
  PushConst(value, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static void CompileLinkage(Val linkage, Compiler *c)
{
  if (linkage == LinkReturn) {
    PushByte(OpReturn, c->chunk);
  } else if (linkage != LinkNext) {
    PushConst(linkage, c->chunk);
    PushByte(OpJump, c->chunk);
  }
}

static CompileResult CompileOk(void)
{
  CompileResult result = {true, 0, 0};
  return result;
}

static CompileResult CompileError(char *message, Compiler *c)
{
  CompileResult result;
  result.ok = false;
  result.error = message;
  result.pos = c->lex.pos;
  return result;
}

void PrintCompileError(CompileResult error, Compiler *c)
{
  Token token;
  char *line, *end, *i;
  u32 line_num;

  InitLexer(&c->lex, c->lex.source, error.pos);

  token = c->lex.token;
  line = token.lexeme;
  end = token.lexeme;
  i = c->lex.source;
  line_num = 0;

  while (line > c->lex.source && *(line-1) != '\n') line--;
  while (*end != '\n' && *end != 0) end++;
  while (i < line) {
    if (*i == '\n') line_num++;
    i++;
  }

  printf("Error: %s\n", error.error);
  printf("%3d⏐ %.*s\n", line_num+1, (i32)(end - line), line);
  printf("     ");
  for (i = line; i < token.lexeme; i++) printf(" ");
  for (i = token.lexeme; i < token.lexeme + token.length; i++) printf("^");
  printf("\n");
}
