#include "parse.h"
#include "lex.h"
#include <stdio.h>
#include <stdlib.h>

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

static Val ParseScript(Parser *p);
static Val ParseStmt(Parser *p);
static Val ParseLetAssigns(Val assigns, Parser *p);
static Val ParseDefAssigns(Val assigns, Parser *p);
static Val ParseCall(Parser *p);
static Val ParseExpr(Prec prec, Parser *p);
static Val ParseLeftAssoc(Val prefix, Parser *p);
static Val ParseRightAssoc(Val prefix, Parser *p);
static Val ParseUnary(Parser *p);
static Val ParseGroup(Parser *p);
static Val ParseDo(Parser *p);
static Val ParseIf(Parser *p);
static Val ParseCond(Parser *p);
static Val ParseList(Parser *p);
static Val ParseTuple(Parser *p);
static Val ParseID(Parser *p);
static Val ParseNum(Parser *p);
static Val ParseString(Parser *p);
static Val ParseSymbol(Parser *p);
static Val ParseLiteral(Parser *p);

typedef Val (*ParseFn)(Parser *p);
typedef Val (*InfixFn)(Val prefix, Parser *p);

typedef struct {
  ParseFn prefix;
  InfixFn infix;
  Prec precedence;
} Rule;

static Rule rules[NumTokenTypes] = {
  [TokenEOF]          = {0,             0,                  PrecNone},
  [TokenID]           = {ParseID,       0,                  PrecNone},
  [TokenBangEqual]    = {0,             ParseLeftAssoc,     PrecEqual},
  [TokenString]       = {ParseString,   0,                  PrecNone},
  [TokenNewline]      = {0,             0,                  PrecNone},
  [TokenHash]         = {ParseUnary,    0,                  PrecNone},
  [TokenPercent]      = {0,             ParseLeftAssoc,     PrecProduct},
  [TokenLParen]       = {ParseGroup,    0,                  PrecNone},
  [TokenRParen]       = {0,             0,                  PrecNone},
  [TokenStar]         = {0,             ParseLeftAssoc,     PrecProduct},
  [TokenPlus]         = {0,             ParseLeftAssoc,     PrecSum},
  [TokenComma]        = {0,             0,                  PrecNone},
  [TokenMinus]        = {ParseUnary,    ParseLeftAssoc,     PrecSum},
  [TokenArrow]        = {0,             0,                  PrecNone},
  [TokenDot]          = {0,             ParseLeftAssoc,     PrecAccess},
  [TokenSlash]        = {0,             ParseLeftAssoc,     PrecProduct},
  [TokenNum]          = {ParseNum,      0,                  PrecNone},
  [TokenColon]        = {ParseSymbol,   0,                  PrecNone},
  [TokenLess]         = {0,             ParseLeftAssoc,     PrecCompare},
  [TokenLessEqual]    = {0,             ParseLeftAssoc,     PrecCompare},
  [TokenEqual]        = {0,             0,                  PrecNone},
  [TokenEqualEqual]   = {0,             ParseLeftAssoc,     PrecEqual},
  [TokenGreater]      = {0,             ParseLeftAssoc,     PrecCompare},
  [TokenGreaterEqual] = {0,             ParseLeftAssoc,     PrecCompare},
  [TokenLBracket]     = {ParseList,     0,                  PrecNone},
  [TokenRBracket]     = {0,             0,                  PrecNone},
  [TokenAnd]          = {0,             ParseLeftAssoc,     PrecLogic},
  [TokenAs]           = {0,             0,                  PrecNone},
  [TokenCond]         = {ParseCond,     0,                  PrecNone},
  [TokenDef]          = {0,             0,                  PrecNone},
  [TokenDo]           = {ParseDo,       0,                  PrecNone},
  [TokenElse]         = {0,             0,                  PrecNone},
  [TokenEnd]          = {0,             0,                  PrecNone},
  [TokenFalse]        = {ParseLiteral,  0,                  PrecNone},
  [TokenIf]           = {ParseIf,       0,                  PrecNone},
  [TokenImport]       = {0,             0,                  PrecNone},
  [TokenIn]           = {0,             ParseLeftAssoc,     PrecMember},
  [TokenLet]          = {0,             0,                  PrecNone},
  [TokenModule]       = {0,             0,                  PrecNone},
  [TokenNil]          = {ParseLiteral,  0,                  PrecNone},
  [TokenNot]          = {ParseUnary,    0,                  PrecNone},
  [TokenOr]           = {0,             ParseLeftAssoc,     PrecLogic},
  [TokenTrue]         = {ParseLiteral,  0,                  PrecNone},
  [TokenLBrace]       = {ParseTuple,    0,                  PrecNone},
  [TokenBar]          = {0,             ParseRightAssoc,    PrecPair},
  [TokenRBrace]       = {0,             0,                  PrecNone}
};

#define ExprNext(lex)   (rules[(lex)->token.type].prefix)
#define PrecNext(lex)   (rules[(lex)->token.type].precedence)

void InitParser(Parser *p)
{
  InitSymbolTable(&p->symbols);
  InitMem(&p->mem, 1024);
}

void DestroyParser(Parser *p)
{
  DestroySymbolTable(&p->symbols);
  DestroyMem(&p->mem);
}

Val Parse(char *source, Parser *p)
{
  InitLexer(&p->lex, source);
  return ParseScript(p);
}

static Val ParseScript(Parser *p)
{
  Val stmts = Nil;
  SkipNewlines(&p->lex);
  while (!MatchToken(TokenEOF, &p->lex)) {
    Val stmt = ParseStmt(p);
    if (stmt == Error) return Error;

    stmts = Pair(stmt, stmts, &p->mem);
    SkipNewlines(&p->lex);
  }

  return Pair(Sym("do", &p->symbols), ReverseList(stmts, &p->mem), &p->mem);
}

static Val ParseStmt(Parser *p)
{
  Val assigns;
  switch (p->lex.token.type) {
  case TokenLet:
    assigns = ParseLetAssigns(Nil, p);
    if (assigns == Error) return Error;
    return Pair(Sym("let", &p->symbols), ReverseList(assigns, &p->mem), &p->mem);
  case TokenDef:
    assigns = ParseDefAssigns(Nil, p);
    if (assigns == Error) return Error;
    return Pair(Sym("let", &p->symbols), ReverseList(assigns, &p->mem), &p->mem);
  default:
    return ParseCall(p);
  }
}

static Val ParseLetAssigns(Val assigns, Parser *p)
{
  while (MatchToken(TokenLet, &p->lex)) {
    while (!MatchToken(TokenNewline, &p->lex)) {
      Val var, val, assign;
      if (MatchToken(TokenEOF, &p->lex)) break;

      var = ParseID(p);
      if (var == Error) return Error;

      if (!MatchToken(TokenEqual, &p->lex)) return Error;
      val = ParseCall(p);
      if (val == Error) return Error;

      assign = Pair(var, Pair(val, Nil, &p->mem), &p->mem);
      assigns = Pair(assign, assigns, &p->mem);

      if (MatchToken(TokenComma, &p->lex)) SkipNewlines(&p->lex);
    }
    SkipNewlines(&p->lex);
  }

  if (p->lex.token.type == TokenDef) return ParseDefAssigns(assigns, p);
  return assigns;
}

static Val ParseDefAssigns(Val assigns, Parser *p)
{
  while (MatchToken(TokenDef, &p->lex)) {
    Val var, params = Nil, body, lambda, assign;
    if (!MatchToken(TokenLParen, &p->lex)) return Error;

    var = ParseID(p);
    if (var == Error) return Error;

    while (!MatchToken(TokenRParen, &p->lex)) {
      Val param = ParseID(p);
      if (param == Error) return Error;
      params = Pair(param, params, &p->mem);
    }

    body = ParseExpr(PrecExpr, p);
    if (body == Error) return Error;

    lambda =
      Pair(Sym("->", &p->symbols),
      Pair(ReverseList(params, &p->mem),
      Pair(body, Nil, &p->mem), &p->mem), &p->mem);

    assign = Pair(var, Pair(lambda, Nil, &p->mem), &p->mem);
    assigns = Pair(assign, assigns, &p->mem);

    SkipNewlines(&p->lex);
  }

  if (p->lex.token.type == TokenLet) return ParseLetAssigns(assigns, p);
  return assigns;
}

static Val ParseCall(Parser *p)
{
  Val op = ParseExpr(PrecExpr, p);
  Val args = Nil;

  if (op == Error) return Error;

  while (p->lex.token.type != TokenNewline
      && p->lex.token.type != TokenComma
      && p->lex.token.type != TokenEOF) {
    Val arg = ParseExpr(PrecExpr, p);
    if (arg == Error) return Error;

    args = Pair(arg, args, &p->mem);
  }

  if (args == Nil) return op;
  return Pair(op, ReverseList(args, &p->mem), &p->mem);
}

static Val ParseExpr(Prec prec, Parser *p)
{
  Val expr;

  if (!ExprNext(&p->lex)) return Error;
  expr = ExprNext(&p->lex)(p);

  while (expr != Error && PrecNext(&p->lex) >= prec) {
    expr = rules[p->lex.token.type].infix(expr, p);
  }

  return expr;
}

static Val ParseLeftAssoc(Val prefix, Parser *p)
{
  Token token = NextToken(&p->lex);
  Prec prec = rules[token.type].precedence;
  Val op = MakeSymbol(token.lexeme, token.length, &p->symbols);
  Val arg = ParseExpr(prec+1, p);
  if (arg == Error) return Error;

  return Pair(op, Pair(prefix, Pair(arg, Nil, &p->mem), &p->mem), &p->mem);
}

static Val ParseRightAssoc(Val prefix, Parser *p)
{
  Token token = NextToken(&p->lex);
  Prec prec = rules[token.type].precedence;
  Val op = MakeSymbol(token.lexeme, token.length, &p->symbols);
  Val arg = ParseExpr(prec, p);
  if (arg == Error) return Error;

  return Pair(op, Pair(prefix, Pair(arg, Nil, &p->mem), &p->mem), &p->mem);
}

static Val ParseUnary(Parser *p)
{
  Token token = NextToken(&p->lex);
  Val op = MakeSymbol(token.lexeme, token.length, &p->symbols);
  Val expr = ParseExpr(PrecUnary, p);
  if (expr == Error) return Error;

  return Pair(op, Pair(expr, Nil, &p->mem), &p->mem);
}

static Val ParseParams(Parser *p)
{
  Val params = Nil;
  Lexer saved = p->lex;

  while (p->lex.token.type == TokenID) {
    Token token = NextToken(&p->lex);
    Val var = MakeSymbol(token.lexeme, token.length, &p->symbols);
    params = Pair(var, params, &p->mem);
  }

  if (MatchToken(TokenRParen, &p->lex) && MatchToken(TokenArrow, &p->lex)) {
    return ReverseList(params, &p->mem);
  }

  p->lex = saved;
  return Error;
}

static Val ParseGroup(Parser *p)
{
  Val expr;
  Assert(MatchToken(TokenLParen, &p->lex));

  expr = ParseParams(p);
  if (expr != Error) {
    Val body = ParseExpr(PrecExpr, p);
    if (body == Error) return Error;

    return Pair(Sym("->", &p->symbols), Pair(expr, Pair(body, Nil, &p->mem), &p->mem), &p->mem);
  }

  expr = Nil;
  SkipNewlines(&p->lex);
  while (!MatchToken(TokenRParen, &p->lex)) {
    Val arg = ParseExpr(PrecExpr, p);
    if (arg == Error) return Error;
    expr = Pair(arg, expr, &p->mem);
    SkipNewlines(&p->lex);
  }
  return ReverseList(expr, &p->mem);
}

static Val ParseDo(Parser *p)
{
  Val stmts = Nil;
  Assert(MatchToken(TokenDo, &p->lex));

  SkipNewlines(&p->lex);
  while (!MatchToken(TokenEnd, &p->lex)) {
    Val stmt = ParseStmt(p);
    if (stmt == Error) return Error;
    stmts = Pair(stmt, stmts, &p->mem);
    SkipNewlines(&p->lex);
  }

  if (Tail(stmts, &p->mem) == Nil) return Head(stmts, &p->mem);
  return stmts;
}

static Val ParseIf(Parser *p)
{
  Val pred, cons = Nil, alt = Nil;
  Assert(MatchToken(TokenIf, &p->lex));

  pred = ParseExpr(PrecExpr, p);
  if (pred == Error) return Error;

  if (!MatchToken(TokenDo, &p->lex)) return Error;
  SkipNewlines(&p->lex);
  while (p->lex.token.type != TokenElse && p->lex.token.type != TokenEnd) {
    Val stmt = ParseStmt(p);
    if (stmt == Error) return Error;
    cons = Pair(stmt, cons, &p->mem);

    SkipNewlines(&p->lex);
  }
  if (Tail(cons, &p->mem) == Nil) {
    cons = Head(cons, &p->mem);
  } else {
    cons = Pair(Sym("do", &p->symbols), ReverseList(cons, &p->mem), &p->mem);
  }

  if (MatchToken(TokenElse, &p->lex)) {
    SkipNewlines(&p->lex);
    while (p->lex.token.type != TokenEnd) {
      Val stmt = ParseStmt(p);
      if (stmt == Error) return Error;
      alt = Pair(stmt, alt, &p->mem);

      SkipNewlines(&p->lex);
    }
    if (Tail(alt, &p->mem) == Nil) {
      alt = Head(alt, &p->mem);
    } else {
      alt = Pair(Sym("do", &p->symbols), ReverseList(alt, &p->mem), &p->mem);
    }
  }

  if (!MatchToken(TokenEnd, &p->lex)) return Error;

  return
    Pair(Sym("if", &p->symbols),
    Pair(pred,
    Pair(cons,
    Pair(alt, Nil, &p->mem), &p->mem), &p->mem), &p->mem);
}

static Val ParseCond(Parser *p)
{
  return Error;
}

static Val ParseList(Parser *p)
{
  Val items = Nil;
  Assert(MatchToken(TokenLBrace, &p->lex));
  while (!MatchToken(TokenRBrace, &p->lex)) {
    Val item = ParseExpr(PrecExpr, p);
    if (item == Error) return Error;

    items = Pair(item, items, &p->mem);
  }

  return Pair(Sym("[]", &p->symbols), items, &p->mem);
}

static Val ParseTuple(Parser *p)
{
  Val items = Nil;
  Assert(MatchToken(TokenLBracket, &p->lex));
  while (!MatchToken(TokenRBracket, &p->lex)) {
    Val item = ParseExpr(PrecExpr, p);
    if (item == Error) return Error;

    items = Pair(item, items, &p->mem);
  }

  return Pair(Sym("{}", &p->symbols), items, &p->mem);
}

static bool ParseID(Parser *p)
{
  Token token = NextToken(&p->lex);
  if (token.type != TokenID) return false;
  return MakeSymbol(token.lexeme, token.length, &p->symbols);
}

static bool ParseNum(Parser *p)
{
  Token token = NextToken(&p->lex);
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
    return FloatVal(num);
  } else {
    return IntVal(whole);
  }
}

static Val ParseString(Parser *p)
{
  Token token = NextToken(&p->lex);
  Val symbol = MakeSymbol(token.lexeme + 1, token.length - 2, &p->symbols);
  return Pair(Sym("\"\"", &p->symbols), Pair(symbol, Nil, &p->mem), &p->mem);
}

static Val ParseSymbol(Parser *p)
{
  Val expr;
  Assert(MatchToken(TokenColon, &p->lex));
  expr = ParseID(p);
  if (expr == Error) return Error;

  return Pair(Sym(":", &p->symbols), Pair(expr, Nil, &p->mem), &p->mem);
}

static Val ParseLiteral(Parser *p)
{
  Token token = NextToken(&p->lex);
  switch (token.type) {
  case TokenTrue:
    return True;
    break;
  case TokenFalse:
    return False;
    break;
  case TokenNil:
    return Sym("nil", &p->symbols);
    break;
  default:
    return Error;
  }
}

void PrintAST(Val ast, u32 level, Mem *mem, SymbolTable *symbols)
{
  if (IsNil(ast)) {
    u32 i;
    for (i = 0; i < level; i++) printf("  ");
  } else if (IsPair(ast)) {
    u32 i;
    for (i = 0; i < level; i++) printf("  ");
    printf("(\n");
    while (!IsNil(ast)) {
      PrintAST(Head(ast, mem), level + 1, mem, symbols);
      ast = Tail(ast, mem);
    }
    for (i = 0; i < level; i++) printf("  ");
    printf(")\n");
  } else {
    u32 i;
    for (i = 0; i < level; i++) printf("  ");
    PrintVal(ast, symbols);
    printf("\n");
  }
}

void PrintParseError(Parser *p)
{
  Token token = p->lex.token;
  char *line = token.lexeme, *end = token.lexeme, *i = p->lex.source;
  u32 line_num = 0;

  while (line > p->lex.source && *(line-1) != '\n') line--;
  while (*end != '\n' && *end != 0) end++;
  while (i < line) {
    if (*i == '\n') line_num++;
    i++;
  }

  printf("Parse error:\n");
  printf("%3d⏐ %.*s\n", line_num, (i32)(end - line), line);
  printf("     ");
  for (i = line; i < token.lexeme; i++) printf(" ");
  for (i = token.lexeme; i < token.lexeme + token.length; i++) printf("^");
  printf("\n");
}

