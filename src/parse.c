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

static Val ParseScript(Compiler *p);
static Val ParseStmt(Compiler *p);
static Val ParseLetAssigns(Val assigns, Compiler *p);
static Val ParseDefAssigns(Val assigns, Compiler *p);
static Val ParseCall(Compiler *p);
static Val ParseExpr(Precedence prec, Compiler *p);
static Val ParseLambda(Val prefix, Compiler *p);
static Val ParseLeftAssoc(Val prefix, Compiler *p);
static Val ParseRightAssoc(Val prefix, Compiler *p);
static Val ParseUnary(Compiler *p);
static Val ParseGroup(Compiler *p);
static Val ParseDo(Compiler *p);
static Val ParseIf(Compiler *p);
static Val ParseCond(Compiler *p);
static Val ParseList(Compiler *p);
static Val ParseTuple(Compiler *p);
static Val ParseID(Compiler *p);
static Val ParseVar(Compiler *p);
static Val ParseNum(Compiler *p);
static Val ParseString(Compiler *p);
static Val ParseSymbol(Compiler *p);
static Val ParseLiteral(Compiler *p);

typedef Val (*ParseFn)(Compiler *p);
typedef Val (*InfixFn)(Val prefix, Compiler *p);

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

void InitParser(Compiler *p)
{
  InitSymbolTable(&p->symbols);
  InitMem(&p->mem, 1024);
  MakeParseSyms(&p->symbols);
}

void DestroyParser(Compiler *p)
{
  DestroySymbolTable(&p->symbols);
  DestroyMem(&p->mem);
}

Val Parse(char *source, Compiler *p)
{
  InitLexer(&p->lex, source, 0);
  return ParseScript(p);
}

static Val ParseScript(Compiler *p)
{
  Val stmts = Nil;
  SkipNewlines(&p->lex);
  while (!MatchToken(TokenEOF, &p->lex)) {
    Val stmt = ParseStmt(p);
    if (stmt == Error) return Error;

    stmts = Pair(stmt, stmts, &p->mem);
    SkipNewlines(&p->lex);
  }

  return MakeNode(SymDo, 0, ReverseList(stmts, &p->mem), &p->mem);
}

static Val ParseStmt(Compiler *p)
{
  Val assigns;
  u32 pos = p->lex.token.lexeme - p->lex.source;

  switch (p->lex.token.type) {
  case TokenLet:
    assigns = ParseLetAssigns(Nil, p);
    if (assigns == Error) return Error;
    return MakeNode(SymLet, pos, ReverseList(assigns, &p->mem), &p->mem);
  case TokenDef:
    assigns = ParseDefAssigns(Nil, p);
    if (assigns == Error) return Error;
    return MakeNode(SymLet, pos, ReverseList(assigns, &p->mem), &p->mem);
  default:
    return ParseCall(p);
  }
}

static Val ParseLetAssigns(Val assigns, Compiler *p)
{
  while (MatchToken(TokenLet, &p->lex)) {
    SkipNewlines(&p->lex);
    while (!MatchToken(TokenNewline, &p->lex)) {
      Val var, val, assign;
      if (MatchToken(TokenEOF, &p->lex)) break;

      var = ParseID(p);
      if (var == Error) return Error;

      if (!MatchToken(TokenEqual, &p->lex)) return Error;
      SkipNewlines(&p->lex);
      val = ParseCall(p);
      if (val == Error) return Error;

      assign = Pair(var, val, &p->mem);
      assigns = Pair(assign, assigns, &p->mem);

      if (MatchToken(TokenComma, &p->lex)) SkipNewlines(&p->lex);
    }
    SkipNewlines(&p->lex);
  }

  if (p->lex.token.type == TokenDef) return ParseDefAssigns(assigns, p);
  return assigns;
}

static Val ParseDefAssigns(Val assigns, Compiler *p)
{
  while (MatchToken(TokenDef, &p->lex)) {
    u32 pos = p->lex.token.lexeme - p->lex.source;
    Val var, params = Nil, body, lambda, assign;
    if (!MatchToken(TokenLParen, &p->lex)) return Error;

    var = ParseID(p);
    if (var == Error) return Error;

    while (!MatchToken(TokenRParen, &p->lex)) {
      Val id = ParseID(p);
      if (id == Error) return Error;
      params = Pair(id, params, &p->mem);
    }

    body = ParseExpr(PrecExpr, p);
    if (body == Error) return Error;

    lambda = MakeNode(SymArrow, pos,
      Pair(ReverseList(params, &p->mem), body, &p->mem), &p->mem);

    assign = Pair(var, lambda, &p->mem);
    assigns = Pair(assign, assigns, &p->mem);

    SkipNewlines(&p->lex);
  }

  if (p->lex.token.type == TokenLet) return ParseLetAssigns(assigns, p);
  return assigns;
}

static Val ParseCall(Compiler *p)
{
  u32 pos = p->lex.token.lexeme - p->lex.source;
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
  return MakeNode(SymLParen, pos, Pair(op, ReverseList(args, &p->mem), &p->mem), &p->mem);
}

static Val ParseExpr(Precedence prec, Compiler *p)
{
  Val expr;

  if (!ExprNext(&p->lex)) return Error;
  expr = ExprNext(&p->lex)(p);

  while (expr != Error && PrecNext(&p->lex) >= prec) {
    expr = rules[p->lex.token.type].infix(expr, p);
  }

  return expr;
}

static Val ParseLambda(Val prefix, Compiler *p)
{
  Val params = Nil;
  Val body;
  u32 pos = RawInt(TupleGet(prefix, 1, &p->mem));
  InitLexer(&p->lex, p->lex.source, pos);

  if (!MatchToken(TokenLParen, &p->lex)) return Error;

  while (!MatchToken(TokenRParen, &p->lex)) {
    Val param = ParseID(p);
    if (param == Error) return Error;
    params = Pair(param, params, &p->mem);
  }

  Assert(MatchToken(TokenArrow, &p->lex));

  body = ParseExpr(PrecLambda, p);
  if (body == Error) return Error;

  return MakeNode(SymArrow, pos, Pair(ReverseList(params, &p->mem), body, &p->mem), &p->mem);
}

static Val ParseLeftAssoc(Val prefix, Compiler *p)
{
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  u32 pos = token.lexeme - p->lex.source;
  Val op = TokenSym(token.type);
  Val arg = ParseExpr(prec+1, p);
  if (arg == Error) return Error;

  return MakeNode(op, pos, Pair(prefix, Pair(arg, Nil, &p->mem), &p->mem), &p->mem);
}

static Val ParseRightAssoc(Val prefix, Compiler *p)
{
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  u32 pos = token.lexeme - p->lex.source;
  Val op = TokenSym(token.type);
  Val arg = ParseExpr(prec, p);
  if (arg == Error) return Error;

  return MakeNode(op, pos, Pair(prefix, Pair(arg, Nil, &p->mem), &p->mem), &p->mem);
}

static Val ParseUnary(Compiler *p)
{
  Token token = NextToken(&p->lex);
  u32 pos = token.lexeme - p->lex.source;
  Val op = TokenSym(token.type);
  Val expr = ParseExpr(PrecUnary, p);
  if (expr == Error) return Error;

  return MakeNode(op, pos, Pair(expr, Nil, &p->mem), &p->mem);
}

static Val ParseGroup(Compiler *p)
{
  u32 pos = p->lex.token.lexeme - p->lex.source;
  Val expr = Nil;
  Assert(MatchToken(TokenLParen, &p->lex));

  SkipNewlines(&p->lex);
  while (!MatchToken(TokenRParen, &p->lex)) {
    Val arg = ParseExpr(PrecExpr, p);
    if (arg == Error) return Error;
    expr = Pair(arg, expr, &p->mem);
    SkipNewlines(&p->lex);
  }
  return MakeNode(SymLParen, pos, ReverseList(expr, &p->mem), &p->mem);
}

static Val ParseDo(Compiler *p)
{
  Val stmts = Nil;
  u32 pos = p->lex.token.lexeme - p->lex.source;
  Assert(MatchToken(TokenDo, &p->lex));

  SkipNewlines(&p->lex);
  while (!MatchToken(TokenEnd, &p->lex)) {
    Val stmt = ParseStmt(p);
    if (stmt == Error) return Error;
    stmts = Pair(stmt, stmts, &p->mem);
    SkipNewlines(&p->lex);
  }

  if (Tail(stmts, &p->mem) == Nil) return Head(stmts, &p->mem);
  return MakeNode(SymDo, pos, stmts, &p->mem);
}

static Val ParseIf(Compiler *p)
{
  Val pred, cons = Nil, alt = Nil;
  u32 pos = p->lex.token.lexeme - p->lex.source;
  u32 else_pos;

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
    cons = MakeNode(SymDo, pos, ReverseList(cons, &p->mem), &p->mem);
  }

  else_pos = p->lex.token.lexeme - p->lex.source;
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
      alt = MakeNode(SymDo, else_pos, ReverseList(alt, &p->mem), &p->mem);
    }
  }

  if (!MatchToken(TokenEnd, &p->lex)) return Error;

  return MakeNode(SymIf, pos,
    Pair(pred,
    Pair(cons,
    Pair(alt, Nil, &p->mem), &p->mem), &p->mem), &p->mem);
}

static Val ParseClauses(Compiler *p)
{
  u32 pos = p->lex.token.lexeme - p->lex.source;

  if (MatchToken(TokenEnd, &p->lex)) {
    return MakeNode(SymColon, pos, Nil, &p->mem);
  } else {
    Val pred, cons, alt;
    pred = ParseExpr(PrecLogic, p);
    if (pred == Error) return Error;
    if (!MatchToken(TokenArrow, &p->lex)) return Error;
    SkipNewlines(&p->lex);
    cons = ParseCall(p);
    if (cons == Error) return Error;
    SkipNewlines(&p->lex);
    alt = ParseClauses(p);
    if (alt == Error) return Error;
    return MakeNode(SymIf, pos,
      Pair(pred,
      Pair(cons,
      Pair(alt, Nil, &p->mem), &p->mem), &p->mem), &p->mem);
  }
}

static Val ParseCond(Compiler *p)
{
  Assert(MatchToken(TokenCond, &p->lex));
  if (!MatchToken(TokenDo, &p->lex)) return Error;
  SkipNewlines(&p->lex);
  return ParseClauses(p);
}

static Val ParseList(Compiler *p)
{
  Val items = Nil;
  u32 pos = p->lex.token.lexeme - p->lex.source;

  Assert(MatchToken(TokenLBrace, &p->lex));
  while (!MatchToken(TokenRBrace, &p->lex)) {
    Val item = ParseExpr(PrecExpr, p);
    if (item == Error) return Error;

    items = Pair(item, items, &p->mem);
  }

  return MakeNode(SymLBracket, pos, items, &p->mem);
}

static Val ParseTuple(Compiler *p)
{
  Val items = Nil;
  u32 pos = p->lex.token.lexeme - p->lex.source;

  Assert(MatchToken(TokenLBracket, &p->lex));
  while (!MatchToken(TokenRBracket, &p->lex)) {
    Val item = ParseExpr(PrecExpr, p);
    if (item == Error) return Error;

    items = Pair(item, items, &p->mem);
  }

  return MakeNode(SymLBrace, pos, items, &p->mem);
}

static bool ParseID(Compiler *p)
{
  Token token = NextToken(&p->lex);
  if (token.type != TokenID) return false;
  return MakeSymbol(token.lexeme, token.length, &p->symbols);
}

static Val ParseVar(Compiler *p)
{
  u32 pos = p->lex.token.lexeme - p->lex.source;
  Val id = ParseID(p);
  if (id == Error) return Error;
  return MakeNode(SymID, pos, id, &p->mem);
}

static bool ParseNum(Compiler *p)
{
  Token token = NextToken(&p->lex);
  u32 pos = token.lexeme - p->lex.source;
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
    return MakeNode(SymNum, pos, FloatVal(num), &p->mem);
  } else {
    return MakeNode(SymNum, pos, IntVal(whole), &p->mem);
  }
}

static Val ParseString(Compiler *p)
{
  Token token = NextToken(&p->lex);
  u32 pos = token.lexeme - p->lex.source;
  Val symbol = MakeSymbol(token.lexeme + 1, token.length - 2, &p->symbols);
  return MakeNode(SymString, pos, symbol, &p->mem);
}

static Val ParseSymbol(Compiler *p)
{
  Val expr;
  u32 pos = p->lex.token.lexeme - p->lex.source;
  Assert(MatchToken(TokenColon, &p->lex));
  expr = ParseID(p);
  if (expr == Error) return Error;

  return MakeNode(SymColon, pos, expr, &p->mem);
}

static Val ParseLiteral(Compiler *p)
{
  Token token = NextToken(&p->lex);
  u32 pos = token.lexeme - p->lex.source;

  switch (token.type) {
  case TokenTrue:
    return MakeNode(SymColon, pos, True, &p->mem);
    break;
  case TokenFalse:
    return MakeNode(SymColon, pos, False, &p->mem);
    break;
  case TokenNil:
    return MakeNode(SymColon, pos, Nil, &p->mem);
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
