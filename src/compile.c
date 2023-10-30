#include "compile.h"
#include "parse.h"
#include "ops.h"
#include "env.h"
#include <stdio.h>
#include <stdlib.h>

#define LinkReturn  0x7FD336A7
#define LinkNext    0x7FD9CB70

static Val CompileExpr(Val node, Val linkage, Compiler *c, Chunk *chunk);
static Val CompileDo(Val node, Val linkage, Compiler *c, Chunk *chunk);
static Val CompileAssigns(Val node, Val linkage, Compiler *c, Chunk *chunk);
static Val CompileCall(Val node, Val linkage, Compiler *c, Chunk *chunk);
static Val CompileLambda(Val node, Val linkage, Compiler *c, Chunk *chunk);
static Val CompileIf(Val node, Val linkage, Compiler *c, Chunk *chunk);
static Val CompileAnd(Val node, Val linkage, Compiler *c, Chunk *chunk);
static Val CompileOr(Val node, Val linkage, Compiler *c, Chunk *chunk);
static Val CompileOp(OpCode op, Val node, Val linkage, Compiler *c, Chunk *chunk);
static Val CompileNotOp(OpCode op, Val node, Val linkage, Compiler *c, Chunk *chunk);
static Val CompileAccess(Val node, Val linkage, Compiler *c, Chunk *chunk);
static Val CompileList(Val node, Val linkage, Compiler *c, Chunk *chunk);
static Val CompileTuple(Val node, Val linkage, Compiler *c, Chunk *chunk);
static Val CompileString(Val node, Val linkage, Compiler *c, Chunk *chunk);
static Val CompileVar(Val node, Val linkage, Compiler *c, Chunk *chunk);
static Val CompileConst(Val node, Val linkage, Chunk *chunk);

static void CompileLinkage(Val linkage, Chunk *chunk);
static Val CompileError(char *message, Compiler *c);
static void PrintCompileError(Val error, Compiler *c);

static void InitCompiler(Compiler *c)
{
  c->env = Nil;
  InitMem(&c->mem, 1024);
  InitSymbolTable(&c->symbols);
  MakeParseSyms(&c->symbols);
}

static void DestroyCompiler(Compiler *c)
{
  DestroyMem(&c->mem);
}

Chunk *Compile(char *source)
{
  Chunk *chunk;
  Compiler c;
  Val ast;
  Val result;

  InitCompiler(&c);
  ast = Parse(source, &c);
  if (ast == Error) {
    Val error = CompileError("Parse error", &c);
    PrintCompileError(error, &c);
    DestroyCompiler(&c);
    return 0;
  }

  PrintAST(ast, 0, &c.mem, &c.symbols);

  chunk = malloc(sizeof(Chunk));
  InitChunk(chunk);

  result = CompileExpr(ast, LinkNext, &c, chunk);
  if (result == Ok) {
    DestroyCompiler(&c);
    return chunk;
  } else {
    PrintCompileError(result, &c);
    DestroyCompiler(&c);
    DestroyChunk(chunk);
    free(chunk);
    return 0;
  }
}

static Val CompileExpr(Val node, Val linkage, Compiler *c, Chunk *chunk)
{
  Val tag = TupleGet(node, 0, &c->mem);
  Val expr = TupleGet(node, 2, &c->mem);
  c->lex.pos = RawInt(TupleGet(node, 1, &c->mem));

  switch (tag) {
  case SymBangEqual:
    return CompileOp(OpEq, expr, linkage, c, chunk);
  case SymString:
    return CompileString(expr, linkage, c, chunk);
  case SymHash:
    return CompileOp(OpLen, expr, linkage, c, chunk);
  case SymPercent:
    return CompileOp(OpRem, expr, linkage, c, chunk);
  case SymStar:
    return CompileOp(OpMul, expr, linkage, c, chunk);
  case SymPlus:
    return CompileOp(OpAdd, expr, linkage, c, chunk);
  case SymMinus:
    if (ListLength(expr, &c->mem) == 2) return CompileOp(OpNeg, expr, linkage, c, chunk);
    else return CompileOp(OpSub, expr, linkage, c, chunk);
  case SymArrow:
    return CompileLambda(expr, linkage, c, chunk);
  case SymDot:
    return CompileAccess(expr, linkage, c, chunk);
  case SymSlash:
    return CompileOp(OpDiv, expr, linkage, c, chunk);
  case SymColon:
    return CompileConst(expr, linkage, chunk);
  case SymLess:
    return CompileOp(OpLt, expr, linkage, c, chunk);
  case SymLessEqual:
    return CompileNotOp(OpGt, expr, linkage, c, chunk);
  case SymEqualEqual:
    return CompileOp(OpEq, expr, linkage, c, chunk);
  case SymGreater:
    return CompileOp(OpGt, expr, linkage, c, chunk);
  case SymGreaterEqual:
    return CompileNotOp(OpLt, expr, linkage, c, chunk);
  case SymLBracket:
    return CompileList(expr, linkage, c, chunk);
  case SymAnd:
    return CompileAnd(expr, linkage, c, chunk);
  case SymDo:
    return CompileDo(expr, linkage, c, chunk);
  case SymIf:
    return CompileIf(expr, linkage, c, chunk);
  case SymNil:
    return CompileConst(Nil, linkage, chunk);
  case SymNot:
    return CompileOp(OpNot, expr, linkage, c, chunk);
  case SymOr:
    return CompileOr(expr, linkage, c, chunk);
  case SymLBrace:
    return CompileTuple(expr, linkage, c, chunk);
  case SymLParen:
    return CompileCall(expr, linkage, c, chunk);
  case SymNum:
    return CompileConst(expr, linkage, chunk);
  case SymID:
    return CompileVar(expr, linkage, c, chunk);
  default:
    return CompileError("Unknown expression", c);
  }
}

static Val CompileDo(Val stmts, Val linkage, Compiler *c, Chunk *chunk)
{
  u32 scopes = 0, i;

  while (stmts != Nil) {
    Val stmt = Head(stmts, &c->mem);
    bool is_last = Tail(stmts, &c->mem) == Nil;
    Val stmt_linkage = is_last ? linkage : LinkNext;
    Val tag = TupleGet(stmt, 0, &c->mem);
    Val expr = TupleGet(stmt, 2, &c->mem);

    if (tag == SymLet) {
      Val result = CompileAssigns(expr, stmt_linkage, c, chunk);
      if (result != Ok) return result;

      scopes++;
      if (is_last) {
        PushByte(OpConst, chunk);
        PushConst(Ok, chunk);
      }
    } else {
      Val result = CompileExpr(stmt, stmt_linkage, c, chunk);
      if (result != Ok) return result;

      if (!is_last) {
        PushByte(OpPop, chunk);
      }
    }

    stmts = Tail(stmts, &c->mem);
  }

  /* cast off our accumulated scopes */
  for (i = 0; i < scopes; i++) {
    c->env = Tail(c->env, &c->mem);
  }

  return Ok;
}

static Val CompileAssigns(Val assigns, Val linkage, Compiler *c, Chunk *chunk)
{
  u32 num_assigns = ListLength(assigns, &c->mem);
  u32 i = 0;

  c->env = ExtendEnv(c->env, MakeTuple(num_assigns, &c->mem), &c->mem);
  PushByte(OpTuple, chunk);
  PushConst(IntVal(num_assigns), chunk);
  PushByte(OpExtend, chunk);

  while (assigns != Nil) {
    Val assign = Head(assigns, &c->mem);
    Val var = Head(assign, &c->mem);
    Val value = Tail(assign, &c->mem);
    Val val_linkage = (Tail(assigns, &c->mem) == Nil) ? linkage : LinkNext;
    Val result;
    Define(var, i, c->env, &c->mem);

    result = CompileExpr(value, val_linkage, c, chunk);
    if (result != Ok) return result;

    PushByte(OpDefine, chunk);
    PushConst(IntVal(i), chunk);

    i++;
    assigns = Tail(assigns, &c->mem);
  }

  return Ok;
}

static Val CompileCall(Val call, Val linkage, Compiler *c, Chunk *chunk)
{
  Val op = Head(call, &c->mem);
  Val args = Tail(call, &c->mem);
  u32 num_args = ListLength(args, &c->mem);
  u32 i = 0;
  Val result;

  u32 patch = PushByte(OpNoop, chunk);
  PushByte(OpNoop, chunk);

  PushByte(OpTuple, chunk);
  PushConst(IntVal(num_args), chunk);

  while (args != Nil) {
    Val arg = Head(args, &c->mem);

    result = CompileExpr(arg, LinkNext, c, chunk);
    if (result != Ok) return result;

    PushByte(OpSet, chunk);
    PushByte(IntVal(i), chunk);

    i++;
    args = Tail(args, &c->mem);
  }

  result = CompileExpr(op, LinkNext, c, chunk);
  if (result != Ok) return result;

  PushByte(OpApply, chunk);
  if (linkage == LinkNext) linkage = IntVal(chunk->count - patch);
  if (linkage != LinkReturn) {
    chunk->code[patch] = OpLink;
    PatchChunk(chunk, patch+1, linkage);
  }

  return Ok;
}

static Val CompileLambda(Val expr, Val linkage, Compiler *c, Chunk *chunk)
{
  Val params = Head(expr, &c->mem);
  Val body = Tail(expr, &c->mem);
  Val result;
  u32 patch, i;
  u32 num_params = ListLength(params, &c->mem);

  c->env = ExtendEnv(c->env, MakeTuple(num_params, &c->mem), &c->mem);
  for (i = 0; i < num_params; i++) {
    Val param = Head(params, &c->mem);
    Define(param, i, c->env, &c->mem);
    params = Tail(params, &c->mem);
  }

  patch = PushByte(OpLambda, chunk);
  PushByte(0, chunk);
  PushByte(OpExtend, chunk);

  result = CompileExpr(body, LinkReturn, c, chunk);
  if (result != Ok) return result;

  PatchChunk(chunk, patch+1, IntVal(chunk->count - patch));

  return Ok;
}

static Val CompileIf(Val expr, Val linkage, Compiler *c, Chunk *chunk)
{
  Val pred = Head(expr, &c->mem);
  Val cons = Head(Tail(expr, &c->mem), &c->mem);
  Val alt = Head(Tail(Tail(expr, &c->mem), &c->mem), &c->mem);
  Val result;
  u32 branch, jump;

  result = CompileExpr(pred, LinkNext, c, chunk);
  if (result != Ok) return result;

  branch = PushByte(OpBranch, chunk);
  PushByte(0, chunk);

  PushByte(OpPop, chunk);
  result = CompileExpr(alt, linkage, c, chunk);
  if (result != Ok) return result;
  if (linkage == LinkNext) {
    jump = PushByte(OpJump, chunk);
    PushByte(0, chunk);
  }

  PatchChunk(chunk, branch+1, IntVal(chunk->count - branch));

  PushByte(OpPop, chunk);
  result = CompileExpr(cons, linkage, c, chunk);

  if (linkage == LinkNext) {
    PatchChunk(chunk, jump, IntVal(chunk->count  - jump));
  }

  return Ok;
}

static Val CompileAnd(Val expr, Val linkage, Compiler *c, Chunk *chunk)
{
  Val a = Head(expr, &c->mem);
  Val b = Head(Tail(expr, &c->mem), &c->mem);
  u32 branch, jump;

  Val result = CompileExpr(a, LinkNext, c, chunk);
  if (result != Ok) return result;

  branch = PushByte(OpBranch, chunk);
  PushByte(0, chunk);

  if (linkage == LinkNext) {
    jump = PushByte(OpJump, chunk);
    PushByte(0, chunk);
  } else {
    CompileLinkage(linkage, chunk);
  }

  PatchChunk(chunk, branch+1, IntVal(chunk->count - branch));

  PushByte(OpPop, chunk);
  result = CompileExpr(b, linkage, c, chunk);
  if (result != Ok) return result;

  if (linkage == LinkNext) {
    PatchChunk(chunk, jump+1, IntVal(chunk->count - jump));
  }

  return Ok;
}

static Val CompileOr(Val expr, Val linkage, Compiler *c, Chunk *chunk)
{
  Val a = Head(expr, &c->mem);
  Val b = Head(Tail(expr, &c->mem), &c->mem);
  u32 branch;

  Val result = CompileExpr(a, LinkNext, c, chunk);
  if (result != Ok) return result;

  branch = PushByte(OpBranch, chunk);
  PushByte(0, chunk);

  PushByte(OpPop, chunk);
  result = CompileExpr(b, linkage, c, chunk);
  if (result != Ok) return result;

  if (linkage == LinkNext || linkage == LinkReturn) {
    PatchChunk(chunk, branch+1, IntVal(chunk->count - branch));
    CompileLinkage(linkage, chunk);
  } else {
    PatchChunk(chunk, branch+1, linkage);
  }

  return Ok;
}

static Val CompileOp(OpCode op, Val args, Val linkage, Compiler *c, Chunk *chunk)
{
  while (args != Nil) {
    Val result = CompileExpr(Head(args, &c->mem), LinkNext, c, chunk);
    if (result != Ok) return result;
    args = Tail(args, &c->mem);
  }
  PushByte(op, chunk);
  CompileLinkage(linkage, chunk);
  return Ok;
}

static Val CompileNotOp(OpCode op, Val args, Val linkage, Compiler *c, Chunk *chunk)
{
  while (args != Nil) {
    Val result = CompileExpr(Head(args, &c->mem), LinkNext, c, chunk);
    if (result != Ok) return result;
  }
  PushByte(op, chunk);
  PushByte(OpNot, chunk);
  CompileLinkage(linkage, chunk);
  return Ok;
}

static Val CompileAccess(Val node, Val linkage, Compiler *c, Chunk *chunk)
{
  return CompileError("Unimplemented", c);
}

static Val CompileList(Val items, Val linkage, Compiler *c, Chunk *chunk)
{
  PushByte(OpConst, chunk);
  PushConst(Nil, chunk);

  while (items != Nil) {
    Val item = Head(items, &c->mem);
    Val result = CompileExpr(item, LinkNext, c, chunk);
    if (result != Ok) return result;
    PushByte(OpPair, chunk);
    items = Tail(items, &c->mem);
  }

  CompileLinkage(linkage, chunk);
  return Ok;
}

static Val CompileTuple(Val items, Val linkage, Compiler *c, Chunk *chunk)
{
  u32 length = ListLength(items, &c->mem);
  u32 i = 0;

  PushByte(OpTuple, chunk);
  PushConst(IntVal(length), chunk);

  while (items != Nil) {
    Val item = Head(items, &c->mem);
    Val result = CompileExpr(item, LinkNext, c, chunk);
    if (result != Ok) return result;
    PushByte(OpSet, chunk);
    PushConst(IntVal(i), chunk);
    i++;
    items = Tail(items, &c->mem);
  }

  CompileLinkage(linkage, chunk);
  return Ok;
}

static Val CompileString(Val sym, Val linkage, Compiler *c, Chunk *chunk)
{
  PushByte(OpConst, chunk);
  PushConst(sym, chunk);
  PushByte(OpStr, chunk);
  CompileLinkage(linkage, chunk);
  return Ok;
}

static Val CompileVar(Val id, Val linkage, Compiler *c, Chunk *chunk)
{
  i32 def = FindDefinition(id, c->env, &c->mem);
  if (def == -1) return CompileError("Undefined variable", c);

  PushByte(OpLookup, chunk);
  PushConst(IntVal(def >> 16), chunk);
  PushConst(IntVal(def & 0xFFFF), chunk);
  CompileLinkage(linkage, chunk);
  return Ok;
}

static Val CompileConst(Val value, Val linkage, Chunk *chunk)
{
  PushByte(OpConst, chunk);
  PushConst(value, chunk);
  CompileLinkage(linkage, chunk);
  return Ok;
}

static void CompileLinkage(Val linkage, Chunk *chunk)
{
  if (linkage == LinkReturn) {
    PushByte(OpReturn, chunk);
  } else if (linkage != LinkNext) {
    PushConst(linkage, chunk);
    PushByte(OpJump, chunk);
  }
}

static Val CompileError(char *message, Compiler *c)
{
  return Pair(Sym(message, &c->symbols), IntVal(c->lex.pos), &c->mem);
}

static void PrintCompileError(Val error, Compiler *c)
{
  char *message = SymbolName(Head(error, &c->mem), &c->symbols);
  u32 pos = RawInt(Tail(error, &c->mem));
  Token token;
  char *line, *end, *i;
  u32 line_num;

  InitLexer(&c->lex, c->lex.source, pos);

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

  printf("Error: %s\n", message);
  printf("%3d⏐ %.*s\n", line_num+1, (i32)(end - line), line);
  printf("     ");
  for (i = line; i < token.lexeme; i++) printf(" ");
  for (i = token.lexeme; i < token.lexeme + token.length; i++) printf("^");
  printf("\n");
}
