#include "parse.h"
#include "lex.h"
#include "module.h"
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

static Val ParseStmt(Parser *p);
static Val ParseImport(Parser *p);
static Val ParseLetAssigns(Val assigns, Parser *p);
static Val ParseDefAssigns(Val assigns, Parser *p);
static Val ParseCall(Parser *p);
static Val ParseExpr(Precedence prec, Parser *p);
static Val ParseLambda(Val prefix, Parser *p);
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
static Val ParseVar(Parser *p);
static Val ParseNum(Parser *p);
static Val ParseString(Parser *p);
static Val ParseSymbol(Parser *p);
static Val ParseLiteral(Parser *p);

typedef Val (*ParseFn)(Parser *p);
typedef Val (*InfixFn)(Val prefix, Parser *p);

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

void InitParser(Parser *p, Mem *mem, SymbolTable *symbols)
{
  p->mem = mem;
  p->symbols = symbols;

#ifdef DEBUG
  Assert(ParseError == Sym("*parse-error*", symbols));
#endif
}

Val Parse(char *source, Parser *p)
{
  InitLexer(&p->lex, source, 0);
  return ParseStmt(p);
}

Val ParseModule(char *filename, Parser *p)
{
  Val stmts = Nil, name = Nil, imports = Nil, exports = Nil;
  char *source = ReadFile(filename);
  InitLexer(&p->lex, source, 0);

  SkipNewlines(&p->lex);

  if (MatchToken(TokenModule, &p->lex)) {
    name = ParseID(p);
    if (name == ParseError) {
      free(source);
      return name;
    }
    SkipNewlines(&p->lex);
  }

  while (!MatchToken(TokenEOF, &p->lex)) {
    Val stmt = ParseStmt(p);
    Val type, expr;
    if (stmt == ParseError) {
      free(source);
      return ParseError;
    }

    type = TupleGet(stmt, 0, p->mem);
    expr = TupleGet(stmt, 2, p->mem);
    if (type == SymImport) {
      Val import = Head(expr, p->mem);
      imports = Pair(import, imports, p->mem);
    } else if (type == SymLet) {
      while (expr != Nil) {
        Val assign = Head(expr, p->mem);
        Val export = Head(assign, p->mem);
        exports = Pair(export, exports, p->mem);
        expr = Tail(expr, p->mem);
      }
    }

    stmts = Pair(stmt, stmts, p->mem);
    SkipNewlines(&p->lex);
  }

  stmts = MakeNode(SymDo, 0, ReverseList(stmts, p->mem), p->mem);

  free(source);
  return MakeModule(name, stmts, imports, exports, Sym(filename, p->symbols), p->mem);
}

static Val ParseStmt(Parser *p)
{
  Val assigns;
  u32 pos = p->lex.token.lexeme - p->lex.source;

  switch (p->lex.token.type) {
  case TokenImport:
    return ParseImport(p);
  case TokenLet:
    assigns = ParseLetAssigns(Nil, p);
    if (assigns == ParseError) return ParseError;
    return MakeNode(SymLet, pos, ReverseList(assigns, p->mem), p->mem);
  case TokenDef:
    assigns = ParseDefAssigns(Nil, p);
    if (assigns == ParseError) return ParseError;
    return MakeNode(SymLet, pos, ReverseList(assigns, p->mem), p->mem);
  default:
    return ParseCall(p);
  }
}

static Val ParseImport(Parser *p)
{
  u32 pos = p->lex.token.lexeme - p->lex.source;
  Val mod, alias;
  Assert(MatchToken(TokenImport, &p->lex));

  mod = ParseID(p);

  if (MatchToken(TokenAs, &p->lex)) {
    alias = ParseID(p);
  } else {
    alias = mod;
  }

  return MakeNode(SymImport, pos, Pair(alias, mod, p->mem), p->mem);
}

static Val ParseLetAssigns(Val assigns, Parser *p)
{
  while (MatchToken(TokenLet, &p->lex)) {
    SkipNewlines(&p->lex);
    while (!MatchToken(TokenNewline, &p->lex)) {
      Val var, val, assign;
      if (MatchToken(TokenEOF, &p->lex)) break;

      var = ParseID(p);
      if (var == ParseError) return ParseError;

      if (!MatchToken(TokenEqual, &p->lex)) return ParseError;
      SkipNewlines(&p->lex);
      val = ParseCall(p);
      if (val == ParseError) return ParseError;

      assign = Pair(var, val, p->mem);
      assigns = Pair(assign, assigns, p->mem);

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
    u32 pos = p->lex.token.lexeme - p->lex.source;
    Val var, params = Nil, body, lambda, assign;
    if (!MatchToken(TokenLParen, &p->lex)) return ParseError;

    var = ParseID(p);
    if (var == ParseError) return ParseError;

    while (!MatchToken(TokenRParen, &p->lex)) {
      Val id = ParseID(p);
      if (id == ParseError) return ParseError;
      params = Pair(id, params, p->mem);
    }

    body = ParseExpr(PrecExpr, p);
    if (body == ParseError) return ParseError;

    lambda = MakeNode(SymArrow, pos,
      Pair(ReverseList(params, p->mem), body, p->mem), p->mem);

    assign = Pair(var, lambda, p->mem);
    assigns = Pair(assign, assigns, p->mem);

    SkipNewlines(&p->lex);
  }

  if (p->lex.token.type == TokenLet) return ParseLetAssigns(assigns, p);
  return assigns;
}

static Val ParseCall(Parser *p)
{
  u32 pos = p->lex.token.lexeme - p->lex.source;
  Val op = ParseExpr(PrecExpr, p);
  Val args = Nil;

  if (op == ParseError) return ParseError;

  while (p->lex.token.type != TokenNewline
      && p->lex.token.type != TokenComma
      && p->lex.token.type != TokenEOF) {
    Val arg = ParseExpr(PrecExpr, p);
    if (arg == ParseError) return ParseError;

    args = Pair(arg, args, p->mem);
  }

  if (args == Nil) return op;
  return MakeNode(SymLParen, pos, Pair(op, ReverseList(args, p->mem), p->mem), p->mem);
}

static Val ParseExpr(Precedence prec, Parser *p)
{
  Val expr;

  if (!ExprNext(&p->lex)) return ParseError;
  expr = ExprNext(&p->lex)(p);

  while (expr != ParseError && PrecNext(&p->lex) >= prec) {
    expr = rules[p->lex.token.type].infix(expr, p);
  }

  return expr;
}

static Val ParseLambda(Val prefix, Parser *p)
{
  Val params = Nil;
  Val body;
  u32 pos = RawInt(TupleGet(prefix, 1, p->mem));
  InitLexer(&p->lex, p->lex.source, pos);

  if (!MatchToken(TokenLParen, &p->lex)) return ParseError;

  while (!MatchToken(TokenRParen, &p->lex)) {
    Val param = ParseID(p);
    if (param == ParseError) return ParseError;
    params = Pair(param, params, p->mem);
  }

  Assert(MatchToken(TokenArrow, &p->lex));

  body = ParseExpr(PrecLambda, p);
  if (body == ParseError) return ParseError;

  return MakeNode(SymArrow, pos, Pair(ReverseList(params, p->mem), body, p->mem), p->mem);
}

static Val ParseLeftAssoc(Val prefix, Parser *p)
{
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  u32 pos = token.lexeme - p->lex.source;
  Val op = TokenSym(token.type);
  Val arg = ParseExpr(prec+1, p);
  if (arg == ParseError) return ParseError;

  return MakeNode(op, pos, Pair(prefix, Pair(arg, Nil, p->mem), p->mem), p->mem);
}

static Val ParseRightAssoc(Val prefix, Parser *p)
{
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  u32 pos = token.lexeme - p->lex.source;
  Val op = TokenSym(token.type);
  Val arg = ParseExpr(prec, p);
  if (arg == ParseError) return ParseError;

  return MakeNode(op, pos, Pair(prefix, Pair(arg, Nil, p->mem), p->mem), p->mem);
}

static Val ParseUnary(Parser *p)
{
  Token token = NextToken(&p->lex);
  u32 pos = token.lexeme - p->lex.source;
  Val op = TokenSym(token.type);
  Val expr = ParseExpr(PrecUnary, p);
  if (expr == ParseError) return ParseError;

  return MakeNode(op, pos, Pair(expr, Nil, p->mem), p->mem);
}

static Val ParseGroup(Parser *p)
{
  u32 pos = p->lex.token.lexeme - p->lex.source;
  Val expr = Nil;
  Assert(MatchToken(TokenLParen, &p->lex));

  SkipNewlines(&p->lex);
  while (!MatchToken(TokenRParen, &p->lex)) {
    Val arg = ParseExpr(PrecExpr, p);
    if (arg == ParseError) return ParseError;
    expr = Pair(arg, expr, p->mem);
    SkipNewlines(&p->lex);
  }
  return MakeNode(SymLParen, pos, ReverseList(expr, p->mem), p->mem);
}

static Val ParseDo(Parser *p)
{
  Val stmts = Nil;
  u32 pos = p->lex.token.lexeme - p->lex.source;
  Assert(MatchToken(TokenDo, &p->lex));

  SkipNewlines(&p->lex);
  while (!MatchToken(TokenEnd, &p->lex)) {
    Val stmt = ParseStmt(p);
    if (stmt == ParseError) return ParseError;
    stmts = Pair(stmt, stmts, p->mem);
    SkipNewlines(&p->lex);
  }

  if (Tail(stmts, p->mem) == Nil) return Head(stmts, p->mem);
  return MakeNode(SymDo, pos, ReverseList(stmts, p->mem), p->mem);
}

static Val ParseIf(Parser *p)
{
  Val pred, cons = Nil, alt = Nil;
  u32 pos = p->lex.token.lexeme - p->lex.source;
  u32 else_pos;

  Assert(MatchToken(TokenIf, &p->lex));

  pred = ParseExpr(PrecExpr, p);
  if (pred == ParseError) return ParseError;

  if (!MatchToken(TokenDo, &p->lex)) return ParseError;
  SkipNewlines(&p->lex);
  while (p->lex.token.type != TokenElse && p->lex.token.type != TokenEnd) {
    Val stmt = ParseStmt(p);
    if (stmt == ParseError) return ParseError;
    cons = Pair(stmt, cons, p->mem);

    SkipNewlines(&p->lex);
  }

  if (cons == Nil) {
    cons = MakeNode(SymNil, pos, Nil, p->mem);
  } else if (Tail(cons, p->mem) == Nil) {
    cons = Head(cons, p->mem);
  } else {
    cons = MakeNode(SymDo, pos, ReverseList(cons, p->mem), p->mem);
  }

  else_pos = p->lex.token.lexeme - p->lex.source;
  if (MatchToken(TokenElse, &p->lex)) {
    SkipNewlines(&p->lex);
    while (p->lex.token.type != TokenEnd) {
      Val stmt = ParseStmt(p);
      if (stmt == ParseError) return ParseError;
      alt = Pair(stmt, alt, p->mem);

      SkipNewlines(&p->lex);
    }

    if (alt == Nil) {
      alt = MakeNode(SymNil, pos, Nil, p->mem);
    } else if (Tail(alt, p->mem) == Nil) {
      alt = Head(alt, p->mem);
    } else {
      alt = MakeNode(SymDo, else_pos, ReverseList(alt, p->mem), p->mem);
    }
  } else {
    alt = MakeNode(SymNil, else_pos, Nil, p->mem);
  }

  if (!MatchToken(TokenEnd, &p->lex)) return ParseError;

  return MakeNode(SymIf, pos,
    Pair(pred,
    Pair(cons,
    Pair(alt, Nil, p->mem), p->mem), p->mem), p->mem);
}

static Val ParseClauses(Parser *p)
{
  u32 pos = p->lex.token.lexeme - p->lex.source;

  if (MatchToken(TokenEnd, &p->lex)) {
    return MakeNode(SymColon, pos, Nil, p->mem);
  } else {
    Val pred, cons, alt;
    pred = ParseExpr(PrecLogic, p);
    if (pred == ParseError) return ParseError;
    if (!MatchToken(TokenArrow, &p->lex)) return ParseError;
    SkipNewlines(&p->lex);
    cons = ParseCall(p);
    if (cons == ParseError) return ParseError;
    SkipNewlines(&p->lex);
    alt = ParseClauses(p);
    if (alt == ParseError) return ParseError;
    return MakeNode(SymIf, pos,
      Pair(pred,
      Pair(cons,
      Pair(alt, Nil, p->mem), p->mem), p->mem), p->mem);
  }
}

static Val ParseCond(Parser *p)
{
  Assert(MatchToken(TokenCond, &p->lex));
  if (!MatchToken(TokenDo, &p->lex)) return ParseError;
  SkipNewlines(&p->lex);
  return ParseClauses(p);
}

static Val ParseList(Parser *p)
{
  Val items = Nil;
  u32 pos = p->lex.token.lexeme - p->lex.source;

  Assert(MatchToken(TokenLBracket, &p->lex));
  while (!MatchToken(TokenRBracket, &p->lex)) {
    Val item = ParseExpr(PrecExpr, p);
    if (item == ParseError) return ParseError;

    items = Pair(item, items, p->mem);
  }

  return MakeNode(SymLBracket, pos, items, p->mem);
}

static Val ParseTuple(Parser *p)
{
  Val items = Nil;
  u32 pos = p->lex.token.lexeme - p->lex.source;

  Assert(MatchToken(TokenLBrace, &p->lex));
  while (!MatchToken(TokenRBrace, &p->lex)) {
    Val item = ParseExpr(PrecExpr, p);
    if (item == ParseError) return ParseError;

    items = Pair(item, items, p->mem);
  }

  return MakeNode(SymLBrace, pos, items, p->mem);
}

static Val ParseID(Parser *p)
{
  Token token = NextToken(&p->lex);
  if (token.type != TokenID) return ParseError;
  return MakeSymbol(token.lexeme, token.length, p->symbols);
}

static Val ParseVar(Parser *p)
{
  u32 pos = p->lex.token.lexeme - p->lex.source;
  Val id = ParseID(p);
  if (id == ParseError) return ParseError;
  return MakeNode(SymID, pos, id, p->mem);
}

static Val ParseNum(Parser *p)
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
    return MakeNode(SymNum, pos, FloatVal(num), p->mem);
  } else {
    return MakeNode(SymNum, pos, IntVal(whole), p->mem);
  }
}

static Val ParseString(Parser *p)
{
  Token token = NextToken(&p->lex);
  u32 pos = token.lexeme - p->lex.source;
  Val symbol = MakeSymbol(token.lexeme + 1, token.length - 2, p->symbols);
  return MakeNode(SymString, pos, symbol, p->mem);
}

static Val ParseSymbol(Parser *p)
{
  Val expr;
  u32 pos = p->lex.token.lexeme - p->lex.source;
  Assert(MatchToken(TokenColon, &p->lex));
  expr = ParseID(p);
  if (expr == ParseError) return ParseError;

  return MakeNode(SymColon, pos, expr, p->mem);
}

static Val ParseLiteral(Parser *p)
{
  Token token = NextToken(&p->lex);
  u32 pos = token.lexeme - p->lex.source;

  switch (token.type) {
  case TokenTrue:
    return MakeNode(SymColon, pos, True, p->mem);
    break;
  case TokenFalse:
    return MakeNode(SymColon, pos, False, p->mem);
    break;
  case TokenNil:
    return MakeNode(SymColon, pos, Nil, p->mem);
    break;
  default:
    return ParseError;
  }
}

void MakeParseSyms(SymbolTable *symbols)
{
  Assert(TokenSym(TokenEOF) == Sym("*eof*", symbols));
  Assert(TokenSym(TokenID) == Sym("*id*", symbols));
  Assert(TokenSym(TokenBangEqual) == Sym("!=", symbols));
  Assert(TokenSym(TokenString) == Sym("\"", symbols));
  Assert(TokenSym(TokenNewline) == Sym("*newline*", symbols));
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
    PrintVal(expr, symbols);
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
  case SymImport:
    printf("(import %s as %s)\n", SymbolName(Head(expr, mem), symbols), SymbolName(Tail(expr, mem), symbols));
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
