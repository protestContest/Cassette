#include "parse.h"
#include "lex.h"
#include <stdio.h>
#include <stdlib.h>

typedef enum {
  PrecNone,
  PrecExpr,
  PrecLambda,
  PrecLogic,
  PrecEqual,
  PrecCompare,
  PrecMember,
  PrecPair,
  PrecSum,
  PrecProduct,
  PrecUnary,
  PrecAccess
} Precedence;

static Val ParseScript(Compiler *c);
static Val ParseStmt(Compiler *c);
static Val ParseImport(Compiler *c);
static Val ParseLetAssigns(Val assigns, Compiler *c);
static Val ParseDefAssigns(Val assigns, Compiler *c);
static Val ParseCall(Compiler *c);
static Val ParseExpr(Precedence prec, Compiler *c);
static Val ParseLambda(Val prefix, Compiler *c);
static Val ParseLeftAssoc(Val prefix, Compiler *c);
static Val ParseRightAssoc(Val prefix, Compiler *c);
static Val ParseUnary(Compiler *c);
static Val ParseGroup(Compiler *c);
static Val ParseDo(Compiler *c);
static Val ParseIf(Compiler *c);
static Val ParseCond(Compiler *c);
static Val ParseList(Compiler *c);
static Val ParseTuple(Compiler *c);
static Val ParseID(Compiler *c);
static Val ParseVar(Compiler *c);
static Val ParseNum(Compiler *c);
static Val ParseString(Compiler *c);
static Val ParseSymbol(Compiler *c);
static Val ParseLiteral(Compiler *c);

typedef Val (*ParseFn)(Compiler *c);
typedef Val (*InfixFn)(Val prefix, Compiler *c);

typedef struct {
  Val symbol;
  ParseFn prefix;
  InfixFn infix;
  Precedence prec;
} Rule;

static Rule rules[NumTokenTypes] = {
  [TokenEOF]          = {SymEOF,            0,            0,                  PrecNone},
  [TokenID]           = {SymID,             ParseVar,     0,                  PrecNone},
  [TokenBangEqual]    = {SymBangEqual,      0,            ParseLeftAssoc,     PrecEqual},
  [TokenString]       = {SymString,         ParseString,  0,                  PrecNone},
  [TokenNewline]      = {SymNewline,        0,            0,                  PrecNone},
  [TokenHash]         = {SymHash,           ParseUnary,   0,                  PrecNone},
  [TokenPercent]      = {SymPercent,        0,            ParseLeftAssoc,     PrecProduct},
  [TokenLParen]       = {SymLParen,         ParseGroup,   0,                  PrecNone},
  [TokenRParen]       = {SymRParen,         0,            0,                  PrecNone},
  [TokenStar]         = {SymStar,           0,            ParseLeftAssoc,     PrecProduct},
  [TokenPlus]         = {SymPlus,           0,            ParseLeftAssoc,     PrecSum},
  [TokenComma]        = {SymComma,          0,            0,                  PrecNone},
  [TokenMinus]        = {SymMinus,          ParseUnary,   ParseLeftAssoc,     PrecSum},
  [TokenArrow]        = {SymArrow,          0,            ParseLambda,       PrecLambda},
  [TokenDot]          = {SymDot,            0,            ParseLeftAssoc,     PrecAccess},
  [TokenSlash]        = {SymSlash,          0,            ParseLeftAssoc,     PrecProduct},
  [TokenNum]          = {SymNum,            ParseNum,     0,                  PrecNone},
  [TokenColon]        = {SymColon,          ParseSymbol,  0,                  PrecNone},
  [TokenLess]         = {SymLess,           0,            ParseLeftAssoc,     PrecCompare},
  [TokenLessEqual]    = {SymLessEqual,      0,            ParseLeftAssoc,     PrecCompare},
  [TokenEqual]        = {SymEqual,          0,            0,                  PrecNone},
  [TokenEqualEqual]   = {SymEqualEqual,     0,            ParseLeftAssoc,     PrecEqual},
  [TokenGreater]      = {SymGreater,        0,            ParseLeftAssoc,     PrecCompare},
  [TokenGreaterEqual] = {SymGreaterEqual,   0,            ParseLeftAssoc,     PrecCompare},
  [TokenLBracket]     = {SymLBracket,       ParseList,    0,                  PrecNone},
  [TokenRBracket]     = {SymRBracket,       0,            0,                  PrecNone},
  [TokenAnd]          = {SymAnd,            0,            ParseLeftAssoc,     PrecLogic},
  [TokenAs]           = {SymAs,             0,            0,                  PrecNone},
  [TokenCond]         = {SymCond,           ParseCond,    0,                  PrecNone},
  [TokenDef]          = {SymDef,            0,            0,                  PrecNone},
  [TokenDo]           = {SymDo,             ParseDo,      0,                  PrecNone},
  [TokenElse]         = {SymElse,           0,            0,                  PrecNone},
  [TokenEnd]          = {SymEnd,            0,            0,                  PrecNone},
  [TokenFalse]        = {SymFalse,          ParseLiteral, 0,                  PrecNone},
  [TokenIf]           = {SymIf,             ParseIf,      0,                  PrecNone},
  [TokenImport]       = {SymImport,         0,            0,                  PrecNone},
  [TokenIn]           = {SymIn,             0,            ParseLeftAssoc,     PrecMember},
  [TokenLet]          = {SymLet,            0,            0,                  PrecNone},
  [TokenModule]       = {SymModule,         0,            0,                  PrecNone},
  [TokenNil]          = {SymNil,            ParseLiteral, 0,                  PrecNone},
  [TokenNot]          = {SymNot,            ParseUnary,   0,                  PrecNone},
  [TokenOr]           = {SymOr,             0,            ParseLeftAssoc,     PrecLogic},
  [TokenTrue]         = {SymTrue,           ParseLiteral, 0,                  PrecNone},
  [TokenLBrace]       = {SymLBrace,         ParseTuple,   0,                  PrecNone},
  [TokenBar]          = {SymBar,            0,            ParseRightAssoc,    PrecPair},
  [TokenRBrace]       = {SymRBrace,         0,            0,                  PrecNone}
};

#define ExprNext(lex)   (rules[(lex)->token.type].prefix)
#define PrecNext(lex)   (rules[(lex)->token.type].prec)
#define TokenSym(type)  (rules[type].symbol)

static Val MakeNode(Val sym, u32 position, Val value, Mem *mem);

Val Parse(char *source, Compiler *c)
{
  InitLexer(&c->lex, source, 0);
  return ParseScript(c);
}

static Val ParseScript(Compiler *c)
{
  Val stmts = Nil, mod = Nil;
  SkipNewlines(&c->lex);

  if (MatchToken(TokenModule, &c->lex)) {
    mod = ParseID(c);
    if (mod == Error) return mod;
    SkipNewlines(&c->lex);
  }

  while (!MatchToken(TokenEOF, &c->lex)) {
    Val stmt = ParseStmt(c);
    if (stmt == Error) return Error;

    stmts = Pair(stmt, stmts, &c->mem);
    SkipNewlines(&c->lex);
  }

  stmts = MakeNode(SymDo, 0, ReverseList(stmts, &c->mem), &c->mem);

  if (mod == Nil) {
    return stmts;
  } else {
    return MakeNode(SymModule, 0, Pair(mod, stmts, &c->mem), &c->mem);
  }
}

static Val ParseStmt(Compiler *c)
{
  Val assigns;
  u32 pos = c->lex.token.lexeme - c->lex.source;

  switch (c->lex.token.type) {
  case TokenImport:
    return ParseImport(c);
  case TokenLet:
    assigns = ParseLetAssigns(Nil, c);
    if (assigns == Error) return Error;
    return MakeNode(SymLet, pos, ReverseList(assigns, &c->mem), &c->mem);
  case TokenDef:
    assigns = ParseDefAssigns(Nil, c);
    if (assigns == Error) return Error;
    return MakeNode(SymLet, pos, ReverseList(assigns, &c->mem), &c->mem);
  default:
    return ParseCall(c);
  }
}

static Val ParseImport(Compiler *c)
{
  u32 pos = c->lex.token.lexeme - c->lex.source;
  Val mod, alias, assign;
  Assert(MatchToken(TokenImport, &c->lex));

  mod = ParseID(c);

  if (MatchToken(TokenAs, &c->lex)) {
    alias = ParseID(c);
  } else {
    alias = mod;
  }

  assign = Pair(alias, MakeNode(SymImport, pos, mod, &c->mem), &c->mem);
  return MakeNode(SymLet, pos, Pair(assign, Nil, &c->mem), &c->mem);
}

static Val ParseLetAssigns(Val assigns, Compiler *c)
{
  while (MatchToken(TokenLet, &c->lex)) {
    SkipNewlines(&c->lex);
    while (!MatchToken(TokenNewline, &c->lex)) {
      Val var, val, assign;
      if (MatchToken(TokenEOF, &c->lex)) break;

      var = ParseID(c);
      if (var == Error) return Error;

      if (!MatchToken(TokenEqual, &c->lex)) return Error;
      SkipNewlines(&c->lex);
      val = ParseCall(c);
      if (val == Error) return Error;

      assign = Pair(var, val, &c->mem);
      assigns = Pair(assign, assigns, &c->mem);

      if (MatchToken(TokenComma, &c->lex)) SkipNewlines(&c->lex);
    }
    SkipNewlines(&c->lex);
  }

  if (c->lex.token.type == TokenDef) return ParseDefAssigns(assigns, c);
  return assigns;
}

static Val ParseDefAssigns(Val assigns, Compiler *c)
{
  while (MatchToken(TokenDef, &c->lex)) {
    u32 pos = c->lex.token.lexeme - c->lex.source;
    Val var, params = Nil, body, lambda, assign;
    if (!MatchToken(TokenLParen, &c->lex)) return Error;

    var = ParseID(c);
    if (var == Error) return Error;

    while (!MatchToken(TokenRParen, &c->lex)) {
      Val id = ParseID(c);
      if (id == Error) return Error;
      params = Pair(id, params, &c->mem);
    }

    body = ParseExpr(PrecExpr, c);
    if (body == Error) return Error;

    lambda = MakeNode(SymArrow, pos,
      Pair(ReverseList(params, &c->mem), body, &c->mem), &c->mem);

    assign = Pair(var, lambda, &c->mem);
    assigns = Pair(assign, assigns, &c->mem);

    SkipNewlines(&c->lex);
  }

  if (c->lex.token.type == TokenLet) return ParseLetAssigns(assigns, c);
  return assigns;
}

static Val ParseCall(Compiler *c)
{
  u32 pos = c->lex.token.lexeme - c->lex.source;
  Val op = ParseExpr(PrecExpr, c);
  Val args = Nil;

  if (op == Error) return Error;

  while (c->lex.token.type != TokenNewline
      && c->lex.token.type != TokenComma
      && c->lex.token.type != TokenEOF) {
    Val arg = ParseExpr(PrecExpr, c);
    if (arg == Error) return Error;

    args = Pair(arg, args, &c->mem);
  }

  if (args == Nil) return op;
  return MakeNode(SymLParen, pos, Pair(op, ReverseList(args, &c->mem), &c->mem), &c->mem);
}

static Val ParseExpr(Precedence prec, Compiler *c)
{
  Val expr;

  if (!ExprNext(&c->lex)) return Error;
  expr = ExprNext(&c->lex)(c);

  while (expr != Error && PrecNext(&c->lex) >= prec) {
    expr = rules[c->lex.token.type].infix(expr, c);
  }

  return expr;
}

static Val ParseLambda(Val prefix, Compiler *c)
{
  Val params = Nil;
  Val body;
  u32 pos = RawInt(TupleGet(prefix, 1, &c->mem));
  InitLexer(&c->lex, c->lex.source, pos);

  if (!MatchToken(TokenLParen, &c->lex)) return Error;

  while (!MatchToken(TokenRParen, &c->lex)) {
    Val param = ParseID(c);
    if (param == Error) return Error;
    params = Pair(param, params, &c->mem);
  }

  Assert(MatchToken(TokenArrow, &c->lex));

  body = ParseExpr(PrecLambda, c);
  if (body == Error) return Error;

  return MakeNode(SymArrow, pos, Pair(ReverseList(params, &c->mem), body, &c->mem), &c->mem);
}

static Val ParseLeftAssoc(Val prefix, Compiler *c)
{
  Token token = NextToken(&c->lex);
  Precedence prec = rules[token.type].prec;
  u32 pos = token.lexeme - c->lex.source;
  Val op = TokenSym(token.type);
  Val arg = ParseExpr(prec+1, c);
  if (arg == Error) return Error;

  return MakeNode(op, pos, Pair(prefix, Pair(arg, Nil, &c->mem), &c->mem), &c->mem);
}

static Val ParseRightAssoc(Val prefix, Compiler *c)
{
  Token token = NextToken(&c->lex);
  Precedence prec = rules[token.type].prec;
  u32 pos = token.lexeme - c->lex.source;
  Val op = TokenSym(token.type);
  Val arg = ParseExpr(prec, c);
  if (arg == Error) return Error;

  return MakeNode(op, pos, Pair(prefix, Pair(arg, Nil, &c->mem), &c->mem), &c->mem);
}

static Val ParseUnary(Compiler *c)
{
  Token token = NextToken(&c->lex);
  u32 pos = token.lexeme - c->lex.source;
  Val op = TokenSym(token.type);
  Val expr = ParseExpr(PrecUnary, c);
  if (expr == Error) return Error;

  return MakeNode(op, pos, Pair(expr, Nil, &c->mem), &c->mem);
}

static Val ParseGroup(Compiler *c)
{
  u32 pos = c->lex.token.lexeme - c->lex.source;
  Val expr = Nil;
  Assert(MatchToken(TokenLParen, &c->lex));

  SkipNewlines(&c->lex);
  while (!MatchToken(TokenRParen, &c->lex)) {
    Val arg = ParseExpr(PrecExpr, c);
    if (arg == Error) return Error;
    expr = Pair(arg, expr, &c->mem);
    SkipNewlines(&c->lex);
  }
  return MakeNode(SymLParen, pos, ReverseList(expr, &c->mem), &c->mem);
}

static Val ParseDo(Compiler *c)
{
  Val stmts = Nil;
  u32 pos = c->lex.token.lexeme - c->lex.source;
  Assert(MatchToken(TokenDo, &c->lex));

  SkipNewlines(&c->lex);
  while (!MatchToken(TokenEnd, &c->lex)) {
    Val stmt = ParseStmt(c);
    if (stmt == Error) return Error;
    stmts = Pair(stmt, stmts, &c->mem);
    SkipNewlines(&c->lex);
  }

  if (Tail(stmts, &c->mem) == Nil) return Head(stmts, &c->mem);
  return MakeNode(SymDo, pos, ReverseList(stmts, &c->mem), &c->mem);
}

static Val ParseIf(Compiler *c)
{
  Val pred, cons = Nil, alt = Nil;
  u32 pos = c->lex.token.lexeme - c->lex.source;
  u32 else_pos;

  Assert(MatchToken(TokenIf, &c->lex));

  pred = ParseExpr(PrecExpr, c);
  if (pred == Error) return Error;

  if (!MatchToken(TokenDo, &c->lex)) return Error;
  SkipNewlines(&c->lex);
  while (c->lex.token.type != TokenElse && c->lex.token.type != TokenEnd) {
    Val stmt = ParseStmt(c);
    if (stmt == Error) return Error;
    cons = Pair(stmt, cons, &c->mem);

    SkipNewlines(&c->lex);
  }
  if (Tail(cons, &c->mem) == Nil) {
    cons = Head(cons, &c->mem);
  } else {
    cons = MakeNode(SymDo, pos, ReverseList(cons, &c->mem), &c->mem);
  }

  else_pos = c->lex.token.lexeme - c->lex.source;
  if (MatchToken(TokenElse, &c->lex)) {
    SkipNewlines(&c->lex);
    while (c->lex.token.type != TokenEnd) {
      Val stmt = ParseStmt(c);
      if (stmt == Error) return Error;
      alt = Pair(stmt, alt, &c->mem);

      SkipNewlines(&c->lex);
    }
    if (Tail(alt, &c->mem) == Nil) {
      alt = Head(alt, &c->mem);
    } else {
      alt = MakeNode(SymDo, else_pos, ReverseList(alt, &c->mem), &c->mem);
    }
  } else {
    alt = MakeNode(SymNil, else_pos, Nil, &c->mem);
  }

  if (!MatchToken(TokenEnd, &c->lex)) return Error;

  return MakeNode(SymIf, pos,
    Pair(pred,
    Pair(cons,
    Pair(alt, Nil, &c->mem), &c->mem), &c->mem), &c->mem);
}

static Val ParseClauses(Compiler *c)
{
  u32 pos = c->lex.token.lexeme - c->lex.source;

  if (MatchToken(TokenEnd, &c->lex)) {
    return MakeNode(SymColon, pos, Nil, &c->mem);
  } else {
    Val pred, cons, alt;
    pred = ParseExpr(PrecLogic, c);
    if (pred == Error) return Error;
    if (!MatchToken(TokenArrow, &c->lex)) return Error;
    SkipNewlines(&c->lex);
    cons = ParseCall(c);
    if (cons == Error) return Error;
    SkipNewlines(&c->lex);
    alt = ParseClauses(c);
    if (alt == Error) return Error;
    return MakeNode(SymIf, pos,
      Pair(pred,
      Pair(cons,
      Pair(alt, Nil, &c->mem), &c->mem), &c->mem), &c->mem);
  }
}

static Val ParseCond(Compiler *c)
{
  Assert(MatchToken(TokenCond, &c->lex));
  if (!MatchToken(TokenDo, &c->lex)) return Error;
  SkipNewlines(&c->lex);
  return ParseClauses(c);
}

static Val ParseList(Compiler *c)
{
  Val items = Nil;
  u32 pos = c->lex.token.lexeme - c->lex.source;

  Assert(MatchToken(TokenLBracket, &c->lex));
  while (!MatchToken(TokenRBracket, &c->lex)) {
    Val item = ParseExpr(PrecExpr, c);
    if (item == Error) return Error;

    items = Pair(item, items, &c->mem);
  }

  return MakeNode(SymLBracket, pos, items, &c->mem);
}

static Val ParseTuple(Compiler *c)
{
  Val items = Nil;
  u32 pos = c->lex.token.lexeme - c->lex.source;

  Assert(MatchToken(TokenLBrace, &c->lex));
  while (!MatchToken(TokenRBrace, &c->lex)) {
    Val item = ParseExpr(PrecExpr, c);
    if (item == Error) return Error;

    items = Pair(item, items, &c->mem);
  }

  return MakeNode(SymLBrace, pos, items, &c->mem);
}

static bool ParseID(Compiler *c)
{
  Token token = NextToken(&c->lex);
  if (token.type != TokenID) return false;
  return MakeSymbol(token.lexeme, token.length, &c->chunk->symbols);
}

static Val ParseVar(Compiler *c)
{
  u32 pos = c->lex.token.lexeme - c->lex.source;
  Val id = ParseID(c);
  if (id == Error) return Error;
  return MakeNode(SymID, pos, id, &c->mem);
}

static bool ParseNum(Compiler *c)
{
  Token token = NextToken(&c->lex);
  u32 pos = token.lexeme - c->lex.source;
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
    return MakeNode(SymNum, pos, FloatVal(num), &c->mem);
  } else {
    return MakeNode(SymNum, pos, IntVal(whole), &c->mem);
  }
}

static Val ParseString(Compiler *c)
{
  Token token = NextToken(&c->lex);
  u32 pos = token.lexeme - c->lex.source;
  Val symbol = MakeSymbol(token.lexeme + 1, token.length - 2, &c->chunk->symbols);
  return MakeNode(SymString, pos, symbol, &c->mem);
}

static Val ParseSymbol(Compiler *c)
{
  Val expr;
  u32 pos = c->lex.token.lexeme - c->lex.source;
  Assert(MatchToken(TokenColon, &c->lex));
  expr = ParseID(c);
  if (expr == Error) return Error;

  return MakeNode(SymColon, pos, expr, &c->mem);
}

static Val ParseLiteral(Compiler *c)
{
  Token token = NextToken(&c->lex);
  u32 pos = token.lexeme - c->lex.source;

  switch (token.type) {
  case TokenTrue:
    return MakeNode(SymColon, pos, True, &c->mem);
    break;
  case TokenFalse:
    return MakeNode(SymColon, pos, False, &c->mem);
    break;
  case TokenNil:
    return MakeNode(SymColon, pos, Nil, &c->mem);
    break;
  default:
    return Error;
  }
}

void MakeParseSyms(SymbolTable *symbols)
{
  Assert(TokenSym(TokenEOF) == Sym("*eof*", symbols));
  Assert(TokenSym(TokenID) == Sym("*id*", symbols));
  Assert(TokenSym(TokenBangEqual) == Sym("!=", symbols));
  Assert(TokenSym(TokenString) == Sym("\"", symbols));
  Assert(TokenSym(TokenNewline) == Sym("\n", symbols));
  Assert(TokenSym(TokenHash) == Sym("#", symbols));
  Assert(TokenSym(TokenPercent) == Sym("%", symbols));
  Assert(TokenSym(TokenLParen) == Sym("(", symbols));
  Assert(TokenSym(TokenRParen) == Sym(")", symbols));
  Assert(TokenSym(TokenStar) == Sym("*", symbols));
  Assert(TokenSym(TokenPlus) == Sym("+", symbols));
  Assert(TokenSym(TokenComma) == Sym(",", symbols));
  Assert(TokenSym(TokenMinus) == Sym("-", symbols));
  Assert(TokenSym(TokenArrow) == Sym("->", symbols));
  Assert(TokenSym(TokenDot) == Sym(".", symbols));
  Assert(TokenSym(TokenSlash) == Sym("/", symbols));
  Assert(TokenSym(TokenNum) == Sym("*num*", symbols));
  Assert(TokenSym(TokenColon) == Sym(":", symbols));
  Assert(TokenSym(TokenLess) == Sym("<", symbols));
  Assert(TokenSym(TokenLessEqual) == Sym("<=", symbols));
  Assert(TokenSym(TokenEqual) == Sym("=", symbols));
  Assert(TokenSym(TokenEqualEqual) == Sym("==", symbols));
  Assert(TokenSym(TokenGreater) == Sym(">", symbols));
  Assert(TokenSym(TokenGreaterEqual) == Sym(">=", symbols));
  Assert(TokenSym(TokenLBracket) == Sym("[", symbols));
  Assert(TokenSym(TokenRBracket) == Sym("]", symbols));
  Assert(TokenSym(TokenAnd) == Sym("and", symbols));
  Assert(TokenSym(TokenAs) == Sym("as", symbols));
  Assert(TokenSym(TokenCond) == Sym("cond", symbols));
  Assert(TokenSym(TokenDef) == Sym("def", symbols));
  Assert(TokenSym(TokenDo) == Sym("do", symbols));
  Assert(TokenSym(TokenElse) == Sym("else", symbols));
  Assert(TokenSym(TokenEnd) == Sym("end", symbols));
  Assert(TokenSym(TokenFalse) == Sym("false", symbols));
  Assert(TokenSym(TokenIf) == Sym("if", symbols));
  Assert(TokenSym(TokenImport) == Sym("import", symbols));
  Assert(TokenSym(TokenIn) == Sym("in", symbols));
  Assert(TokenSym(TokenLet) == Sym("let", symbols));
  Assert(TokenSym(TokenModule) == Sym("module", symbols));
  Assert(TokenSym(TokenNil) == Sym("nil", symbols));
  Assert(TokenSym(TokenNot) == Sym("not", symbols));
  Assert(TokenSym(TokenOr) == Sym("or", symbols));
  Assert(TokenSym(TokenTrue) == Sym("true", symbols));
  Assert(TokenSym(TokenLBrace) == Sym("{", symbols));
  Assert(TokenSym(TokenBar) == Sym("|", symbols));
  Assert(TokenSym(TokenRBrace) == Sym("}", symbols));
}

void PrintAST(Val node, u32 level, Mem *mem, SymbolTable *symbols)
{
  Val tag = TupleGet(node, 0, mem);
  Val expr = TupleGet(node, 2, mem);

  u32 i;
  for (i = 0; i < level; i++) printf("  ");

  switch (tag) {
  case SymNum:
  case SymID:
    PrintVal(expr, symbols);
    printf("\n");
    break;
  case SymColon:
    printf(":");
    PrintVal(node, symbols);
    printf("\n");
    break;
  case SymString:
    printf("\"");
    PrintVal(node, symbols);
    printf("\"\n");
    break;
  case SymLParen:
    printf("(\n");
    while (expr != Nil) {
      PrintAST(Head(expr, mem), level + 1, mem, symbols);
      expr = Tail(expr, mem);
    }
    for (i = 0; i < level; i++) printf("  ");
    printf(")\n");
    break;
  case SymLet:
    printf("(%s\n", SymbolName(tag, symbols));
    for (i = 0; i < level + 1; i++) printf("  ");
    while (expr != Nil) {
      Val assign = Head(expr, mem);
      Val var = Head(assign, mem);
      Val value = Tail(assign, mem);
      PrintVal(var, symbols);
      printf(" =\n");
      PrintAST(value, level + 2, mem, symbols);
      expr = Tail(expr, mem);
    }
    for (i = 0; i < level; i++) printf("  ");
    printf(")\n");
    break;
  case SymArrow: {
    Val params = Head(expr, mem);
    Val body = Tail(expr, mem);
    printf ("((");
    while (params != Nil) {
      Val param = Head(params, mem);
      PrintVal(param, symbols);
      params = Tail(params, mem);
      if (params != Nil) printf(" ");
    }

    printf(") ->\n");
    PrintAST(body, level + 1, mem, symbols);
    for (i = 0; i < level; i++) printf("  ");
    printf(")\n");
    break;
  }
  default:
    printf("(%s\n", SymbolName(tag, symbols));
    while (expr != Nil) {
      PrintAST(Head(expr, mem), level + 1, mem, symbols);
      expr = Tail(expr, mem);
    }
    for (i = 0; i < level; i++) printf("  ");
    printf(")\n");
    break;
  }
}

static Val MakeNode(Val sym, u32 position, Val value, Mem *mem)
{
  Val node = MakeTuple(3, mem);
  TupleSet(node, 0, sym, mem);
  TupleSet(node, 1, IntVal(position), mem);
  TupleSet(node, 2, value, mem);
  return node;
}
