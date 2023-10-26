#include "compile.h"
#include "lex.h"
#include "ops.h"
#include "env.h"
#include <stdio.h>
#include <stdlib.h>

#define LinkReturn  0x7FD336A7
#define LinkNext    0x7FD9CB70

typedef enum {
  PrecNone,
  PrecExpr,
  PrecLogic,
  PrecEqual,
  PrecCompare,
  PrecMember,
  PrecPair,
  PrecSum,
  PrecProduct,
  PrecUnary,
  PrecAccess
} Prec;

static bool CompileBlock(Compiler *c, Chunk *chunk, Val linkage);
static bool CompileArgs(Compiler *c, Chunk *chunk);
static void CompileApplication(Val linkage, Chunk *chunk);
static bool CompileExpr(Prec prec, Compiler *c, Chunk *chunk, Val linkage);
static Val ParseParams(Compiler *c);
static bool CompileLambda(Compiler *c, Chunk *chunk);
static bool CompileAnd(Compiler *c, Chunk *chunk);
static bool CompileOr(Compiler *c, Chunk *chunk);
static bool CompileLeftAssoc(Compiler *c, Chunk *chunk);
static bool CompileRightAssoc(Compiler *c, Chunk *chunk);
static bool CompileUnary(Compiler *c, Chunk *chunk);
static bool CompileVar(Compiler *c, Chunk *chunk);
static bool CompileNum(Compiler *c, Chunk *chunk);
static void CompileLinkage(Val linkage, Chunk *chunk);

typedef bool (*CompileFn)(Compiler *c, Chunk *chunk);

typedef struct {
  CompileFn prefix;
  CompileFn infix;
  Prec precedence;
} Rule;

static Rule rules[NumTokenTypes] = {
  [TokenEOF]          = {0,             0,                  PrecNone},
  [TokenID]           = {CompileVar,    0,                  PrecNone},
  [TokenBangEqual]    = {0,             CompileLeftAssoc,   PrecEqual},
  [TokenString]       = {0,             0,                  PrecNone},
  [TokenNewline]      = {0,             0,                  PrecNone},
  [TokenHash]         = {CompileUnary,  0,                  PrecNone},
  [TokenPercent]      = {0,             CompileLeftAssoc,   PrecProduct},
  [TokenLParen]       = {0,             0,                  PrecNone},
  [TokenRParen]       = {0,             0,                  PrecNone},
  [TokenStar]         = {0,             CompileLeftAssoc,   PrecProduct},
  [TokenPlus]         = {0,             CompileLeftAssoc,   PrecSum},
  [TokenComma]        = {0,             0,                  PrecNone},
  [TokenMinus]        = {CompileUnary,  CompileLeftAssoc,   PrecSum},
  [TokenArrow]        = {0,             0,                  PrecNone},
  [TokenDot]          = {0,             CompileLeftAssoc,   PrecAccess},
  [TokenSlash]        = {0,             CompileLeftAssoc,   PrecProduct},
  [TokenNum]          = {CompileNum,    0,                  PrecNone},
  [TokenColon]        = {0,             0,                  PrecNone},
  [TokenLess]         = {0,             CompileLeftAssoc,   PrecCompare},
  [TokenLessEqual]    = {0,             CompileLeftAssoc,   PrecCompare},
  [TokenEqual]        = {0,             0,                  PrecNone},
  [TokenEqualEqual]   = {0,             CompileLeftAssoc,   PrecEqual},
  [TokenGreater]      = {0,             CompileLeftAssoc,   PrecCompare},
  [TokenGreaterEqual] = {0,             CompileLeftAssoc,   PrecCompare},
  [TokenLBracket]     = {0,             0,                  PrecNone},
  [TokenRBracket]     = {0,             0,                  PrecNone},
  [TokenAnd]          = {0,             CompileAnd,         PrecLogic},
  [TokenAs]           = {0,             0,                  PrecNone},
  [TokenCond]         = {0,             0,                  PrecNone},
  [TokenDef]          = {0,             0,                  PrecNone},
  [TokenDo]           = {0,             0,                  PrecNone},
  [TokenElse]         = {0,             0,                  PrecNone},
  [TokenEnd]          = {0,             0,                  PrecNone},
  [TokenFalse]        = {0,             0,                  PrecNone},
  [TokenIf]           = {0,             0,                  PrecNone},
  [TokenImport]       = {0,             0,                  PrecNone},
  [TokenIn]           = {0,             CompileLeftAssoc,   PrecMember},
  [TokenLet]          = {0,             0,                  PrecNone},
  [TokenModule]       = {0,             0,                  PrecNone},
  [TokenNil]          = {0,             0,                  PrecNone},
  [TokenNot]          = {CompileUnary,  0,                  PrecNone},
  [TokenOr]           = {0,             CompileOr,          PrecLogic},
  [TokenTrue]         = {0,             0,                  PrecNone},
  [TokenLBrace]       = {0,             0,                  PrecNone},
  [TokenBar]          = {0,             CompileRightAssoc,  PrecPair},
  [TokenRBrace]       = {0,             0,                  PrecNone}
};

#define ExprNext(c)   ((c)->lex.token.type == TokenLParen || (rules[(c)->lex.token.type].prefix != 0))

static void InitCompiler(Compiler *c, char *source)
{
  InitLexer(&c->lex, source);
  c->env = Nil;
  InitMem(&c->mem, 1024);
}

static void DestroyCompiler(Compiler *c)
{
  DestroyMem(&c->mem);
}

Chunk *Compile(char *source)
{
  Chunk *chunk = malloc(sizeof(Chunk));
  Compiler c;
  InitChunk(chunk);
  InitCompiler(&c, source);

  if (CompileBlock(&c, chunk, LinkNext)) {
    DestroyCompiler(&c);
    return chunk;
  } else {
    DestroyCompiler(&c);
    DestroyChunk(chunk);
    free(chunk);
    return 0;
  }
}

static bool CompileBlock(Compiler *c, Chunk *chunk, Val linkage)
{
  Chunk op;
  InitChunk(&op);
  SkipNewlines(&c->lex);

  while (ExprNext(c)) {
    /* compile first argument */
    if (!CompileExpr(PrecExpr, c, &op, LinkNext)) return false;

    if (ExprNext(c)) {
      /* stmt has more args, so we want to call it */
      if (!CompileArgs(c, chunk)) return false;
      /* TODO: preserve env */
      AppendChunk(chunk, &op);
      ResetChunk(&op);

      /* if there's another stmt, we need to return here */
      SkipNewlines(&c->lex);
      if (ExprNext(c)) {
        CompileApplication(LinkNext, chunk);
        PushByte(OpPop, chunk);
      } else {
        CompileApplication(linkage, chunk);
      }
    } else {
      /* stand-alone expr, don't call it */
      /* TODO: preserve env */
      AppendChunk(chunk, &op);
      ResetChunk(&op);

      /* if there's another stmt, discard this stmt result */
      SkipNewlines(&c->lex);
      if (ExprNext(c)) {
        PushByte(OpPop, chunk);
      }
    }
  }

  DestroyChunk(&op);

  return true;
}

static bool CompileArgs(Compiler *c, Chunk *chunk)
{
  /* Arguments in a call must be pushed onto the stack in reverse order, so we
     compile into an arg chunk and continually prepend it to an args chunk. Then
     we can tack on the whole call at the end */
  Chunk args;
  Chunk arg, tmp;
  u32 num_args = 0;

  InitChunk(&args);
  InitChunk(&arg);

  while (ExprNext(c)) {
    if (!CompileExpr(PrecExpr, c, &arg, LinkNext)) return false;

    /* swap so that args is the variable that got modified */
    /* TODO: preserve env */
    tmp = args;
    AppendChunk(&arg, &args);
    args = arg;
    arg = tmp;
    ResetChunk(&arg);
    num_args++;
  }
  DestroyChunk(&arg);

  AppendChunk(chunk, &args);
  DestroyChunk(&args);

  /* create a tuple of the args */
  PushConst(IntVal(num_args), chunk);
  PushByte(OpTuple, chunk);

  return true;
}

static void CompileApplication(Val linkage, Chunk *chunk)
{
  if (linkage == LinkReturn) {
    /* preserve return reg before OpApply */
    /* needs return, modifies env */
    PushByte(OpApply, chunk);
  } else {
    if (linkage == LinkNext) {
      /* link to after the call */
      PushConst(IntVal(2), chunk);
    } else {
      PushConst(linkage, chunk);
    }
    /* modifies env, return */
    PushByte(OpLink, chunk);
    PushByte(OpApply, chunk);
  }
}

static bool CompileExpr(Prec prec, Compiler *c, Chunk *chunk, Val linkage)
{
  if (MatchToken(TokenLParen, &c->lex)) {
    /* groups require special care:
      - is it actually a lambda?
      - if not, should we apply it with the given linkage? (i.e. is there an operater after it?)
    */

    Val params = ParseParams(c);
    if (params != Error) {
      /* it's a lambda */
      c->env = ExtendEnv(c->env, params, &c->mem);
      return CompileLambda(c, chunk);
    } else {
      Chunk op;
      InitChunk(&op);
      if (!CompileExpr(PrecExpr, c, &op, LinkNext)) return false;
      if (!CompileArgs(c, chunk)) return false;
      AppendChunk(chunk, &op);
      DestroyChunk(&op);

      Assert(MatchToken(TokenRParen, &c->lex));

      if (rules[c->lex.token.type].precedence < prec) {
        /* this is the only part of the expr, apply with our linkage */
        CompileApplication(linkage, chunk);
        return true;
      } else {
        /* an infix op follows - apply but return here */
        CompileApplication(LinkNext, chunk);
      }
    }
  } else {
    /* not a group, look it up in the rules */
    if (!ExprNext(c)) return false;
    if (!rules[c->lex.token.type].prefix(c, chunk)) return false;
  }

  while (rules[c->lex.token.type].precedence >= prec) {
    if (!rules[c->lex.token.type].infix(c, chunk)) return false;
  }

  CompileLinkage(linkage, chunk);
  return true;
}

static Val ParseParams(Compiler *c)
{
  Val params = Nil, frame;
  u32 num_params = 0, i;
  Lexer saved = c->lex;

  while (c->lex.token.type == TokenID) {
    Token token = NextToken(&c->lex);
    Val var = MakeSymbol(token.lexeme, token.length, &c->mem.symbols);
    params = Pair(var, params, &c->mem);
    num_params++;
  }
  if (!MatchToken(TokenRParen, &c->lex)
        || !MatchToken(TokenArrow, &c->lex)) {
    c->lex = saved;
    return Error;
  }

  frame = MakeTuple(num_params, &c->mem);
  for (i = 0; i < num_params; i++) {
    TupleSet(frame, num_params - 1 - i, Head(params, &c->mem), &c->mem);
    params = Tail(params, &c->mem);
  }

  return frame;
}

static bool CompileLambda(Compiler *c, Chunk *chunk)
{
  u32 patch;

  PushConst(IntVal(0), chunk); /* placeholder */
  patch = chunk->count;
  PushByte(OpLambda, chunk);

  /* lambda body */
  PushByte(OpExtend, chunk);
  if (!CompileExpr(PrecExpr, c, chunk, LinkReturn)) return false;

  c->env = Tail(c->env, &c->mem);
  PatchChunk(chunk, patch-1, IntVal(chunk->count - patch));
  return true;
}

static bool CompileAnd(Compiler *c, Chunk *chunk)
{
  return false;
}

static bool CompileOr(Compiler *c, Chunk *chunk)
{
  return false;
}

static bool PushOp(TokenType type, Chunk *chunk)
{
  switch (type) {
  case TokenBangEqual:
    PushByte(OpEq, chunk);
    PushByte(OpNot, chunk);
    break;
  case TokenPercent:
    PushByte(OpRem, chunk);
    break;
  case TokenStar:
    PushByte(OpMul, chunk);
    break;
  case TokenPlus:
    PushByte(OpAdd, chunk);
    break;
  case TokenMinus:
    PushByte(OpSub, chunk);
    break;
  case TokenSlash:
    PushByte(OpDiv, chunk);
    break;
  case TokenLess:
    PushByte(OpLt, chunk);
    break;
  case TokenLessEqual:
    PushByte(OpGt, chunk);
    PushByte(OpNot, chunk);
    break;
  case TokenEqualEqual:
    PushByte(OpEq, chunk);
    break;
  case TokenGreater:
    PushByte(OpGt, chunk);
    break;
  case TokenGreaterEqual:
    PushByte(OpLt, chunk);
    PushByte(OpNot, chunk);
    break;
  case TokenIn:
    PushByte(OpIn, chunk);
    break;
  case TokenBar:
    PushByte(OpPair, chunk);
    break;
  default:
    return false;
  }
  return true;
}

static bool CompileLeftAssoc(Compiler *c, Chunk *chunk)
{
  Token token = NextToken(&c->lex);
  Prec prec = rules[token.type].precedence;
  if (!CompileExpr(prec+1, c, chunk, LinkNext)) return false;
  return PushOp(token.type, chunk);
}

static bool CompileRightAssoc(Compiler *c, Chunk *chunk)
{
  Token token = NextToken(&c->lex);
  Prec prec = rules[token.type].precedence;
  if (!CompileExpr(prec, c, chunk, LinkNext)) return false;
  return PushOp(token.type, chunk);
}

static bool CompileUnary(Compiler *c, Chunk *chunk)
{
  Token token = NextToken(&c->lex);
  if (!CompileExpr(PrecUnary, c, chunk, LinkNext)) return false;

  switch (token.type) {
  case TokenMinus:
    PushByte(OpNeg, chunk);
    break;
  case TokenNot:
    PushByte(OpNot, chunk);
    break;
  case TokenHash:
    PushByte(OpLen, chunk);
    break;
  default:
    return false;
  }

  return true;
}

static bool CompileVar(Compiler *c, Chunk *chunk)
{
  Token token = NextToken(&c->lex);
  Val var = MakeSymbol(token.lexeme, token.length, &c->mem.symbols);
  i32 def = FindDefinition(var, c->env, &c->mem);

  if (def == -1) return false;

  /* needs env */
  PushByte(OpLookup, chunk);
  PushByte(def >> 8, chunk);
  PushByte(def & 0xFF, chunk);

  return true;
}

static bool CompileNum(Compiler *c, Chunk *chunk)
{
  Token token = NextToken(&c->lex);
  u32 whole = 0, frac = 0, frac_size = 1, i;

  for (i = 0; i < token.length; i++) {
    if (token.lexeme[i] == '.') {
      i++;
      break;
    }
    whole = whole * 10 + token.lexeme[i] - '0';
  }
  for (; i < token.length; i++) {
    frac_size *= 10;
    frac  = frac * 10 + token.lexeme[i] - '0';
  }

  if (frac != 0) {
    float num = (float)whole + (float)frac / (float)frac_size;
    PushConst(FloatVal(num), chunk);
  } else {
    PushConst(IntVal(whole), chunk);
  }

  return true;
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
