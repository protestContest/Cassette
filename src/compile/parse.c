#include "parse.h"
#include "lex.h"
#include "module.h"
#include "source.h"
#include "univ/system.h"
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

static Result ParseStmt(Parser *p);
static Result ParseLetAssigns(Val assigns, Parser *p);
static Result ParseDefAssigns(Val assigns, Parser *p);
static Result ParseCall(Parser *p);
static Result ParseExpr(Precedence prec, Parser *p);
static Result ParseLambda(Val prefix, Parser *p);
static Result ParseLeftAssoc(Val prefix, Parser *p);
static Result ParseRightAssoc(Val prefix, Parser *p);
static Result ParseUnary(Parser *p);
static Result ParseGroup(Parser *p);
static Result ParseDo(Parser *p);
static Result ParseIf(Parser *p);
static Result ParseCond(Parser *p);
static Result ParseList(Parser *p);
static Result ParseTuple(Parser *p);
static Result ParseID(Parser *p);
static Result ParseVar(Parser *p);
static Result ParseNum(Parser *p);
static Result ParseString(Parser *p);
static Result ParseSymbol(Parser *p);
static Result ParseLiteral(Parser *p);

typedef Result (*ParseFn)(Parser *p);
typedef Result (*InfixFn)(Val prefix, Parser *p);

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
static Result ParseError(char *message, Parser *p);
#define ParseOk(val)  OkResult(val)

void InitParser(Parser *p, Mem *mem, SymbolTable *symbols)
{
  p->mem = mem;
  p->symbols = symbols;
  p->imports = Nil;
}

Result ParseModule(char *filename, Parser *p)
{
  Result result;
  Val stmts, name, imports, exports;
  char *source;

  p->filename = filename;
  p->lex.source = 0;
  source = ReadFile(filename);

  if (source == 0) return ParseError("Could not read file", p);
  InitLexer(&p->lex, source, 0);
  SkipNewlines(&p->lex);

  /* parse optional module name */
  if (MatchToken(TokenModule, &p->lex)) {
    result = ParseID(p);
    if (!result.ok) return result;
    name = result.value;
    SkipNewlines(&p->lex);
  } else {
    name = Sym(filename, p->symbols);
  }

  /* parse imports */
  imports = Nil;
  while (MatchToken(TokenImport, &p->lex)) {
    Val import;
    u32 pos = p->lex.token.lexeme - p->lex.source;
    Result result = ParseID(p);
    if (!result.ok) return result;

    import = MakeNode(SymImport, pos, result.value, p->mem);

    SkipNewlines(&p->lex);
    imports = Pair(import, imports, p->mem);
  }
  imports = ReverseList(imports, p->mem);

  stmts = Nil;
  exports = Nil;
  while (!MatchToken(TokenEOF, &p->lex)) {
    result = ParseStmt(p);
    if (!result.ok) return result;

    /* count number of exports in stmt */
    if (NodeType(result.value, p->mem) == SymLet) {
      Val expr = NodeExpr(result.value, p->mem);
      while (expr != Nil) {
        Val assign = Head(expr, p->mem);
        Val export = Head(assign, p->mem);
        exports = Pair(export, exports, p->mem);
        expr = Tail(expr, p->mem);
      }
    }

    stmts = Pair(result.value, stmts, p->mem);
    SkipNewlines(&p->lex);
  }
  stmts = ReverseList(stmts, p->mem);
  exports = ReverseList(exports, p->mem);

  free(source);
  return ParseOk(MakeModule(name, stmts, imports, exports, Sym(filename, p->symbols), p->mem));
}

static Result ParseStmt(Parser *p)
{
  Result result;
  u32 pos = p->lex.token.lexeme - p->lex.source;

  switch (p->lex.token.type) {
  case TokenLet:
    result = ParseLetAssigns(Nil, p);
    if (!result.ok) return result;
    return ParseOk(MakeNode(SymLet, pos, ReverseList(result.value, p->mem), p->mem));
  case TokenDef:
    result = ParseDefAssigns(Nil, p);
    if (!result.ok) return result;
    return ParseOk(MakeNode(SymLet, pos, ReverseList(result.value, p->mem), p->mem));
  default:
    return ParseCall(p);
  }
}

static Result ParseLetAssigns(Val assigns, Parser *p)
{
  Result result;
  while (MatchToken(TokenLet, &p->lex)) {
    SkipNewlines(&p->lex);
    while (!MatchToken(TokenNewline, &p->lex)) {
      Val var, val, assign;
      if (MatchToken(TokenEOF, &p->lex)) break;

      result = ParseID(p);
      if (!result.ok) return result;
      var = result.value;

      if (!MatchToken(TokenEqual, &p->lex)) return ParseError("Expected \"=\"", p);
      SkipNewlines(&p->lex);

      result = ParseCall(p);
      if (!result.ok) return result;
      val = result.value;

      assign = Pair(var, val, p->mem);
      assigns = Pair(assign, assigns, p->mem);

      if (MatchToken(TokenComma, &p->lex)) SkipNewlines(&p->lex);
    }
    SkipNewlines(&p->lex);
  }

  if (p->lex.token.type == TokenDef) return ParseDefAssigns(assigns, p);
  return ParseOk(assigns);
}

static Result ParseDefAssigns(Val assigns, Parser *p)
{
  Result result;
  while (MatchToken(TokenDef, &p->lex)) {
    u32 pos = p->lex.token.lexeme - p->lex.source;
    Val var, params = Nil, body, lambda, assign;
    if (!MatchToken(TokenLParen, &p->lex)) return ParseError("Expected \"(\"", p);

    result = ParseID(p);
    if (!result.ok) return result;
    var = result.value;

    while (!MatchToken(TokenRParen, &p->lex)) {
      result = ParseID(p);
      if (!result.ok) return result;
      params = Pair(result.value, params, p->mem);
    }

    result = ParseExpr(PrecExpr, p);
    if (!result.ok) return result;
    body = result.value;

    lambda = MakeNode(SymArrow, pos,
      Pair(ReverseList(params, p->mem), body, p->mem), p->mem);

    assign = Pair(var, lambda, p->mem);
    assigns = Pair(assign, assigns, p->mem);

    SkipNewlines(&p->lex);
  }

  if (p->lex.token.type == TokenLet) return ParseLetAssigns(assigns, p);
  return ParseOk(assigns);
}

static Result ParseCall(Parser *p)
{
  Result result;
  u32 pos = p->lex.token.lexeme - p->lex.source;
  Val args = Nil;
  Val op;

  result = ParseExpr(PrecExpr, p);
  if (!result.ok) return result;
  op = result.value;

  while (p->lex.token.type != TokenNewline
      && p->lex.token.type != TokenComma
      && p->lex.token.type != TokenEOF) {
    result = ParseExpr(PrecExpr, p);
    if (!result.ok) return result;
    args = Pair(result.value, args, p->mem);
  }

  if (args == Nil) return ParseOk(op);
  return ParseOk(MakeNode(SymLParen, pos, Pair(op, ReverseList(args, p->mem), p->mem), p->mem));
}

static Result ParseExpr(Precedence prec, Parser *p)
{
  Result result;

  if (!ExprNext(&p->lex)) {
    return ParseError("Expected expression", p);
  }
  result = ExprNext(&p->lex)(p);

  while (result.ok && PrecNext(&p->lex) >= prec) {
    result = rules[p->lex.token.type].infix(result.value, p);
  }

  return result;
}

static Result ParseLambda(Val prefix, Parser *p)
{
  Result result;
  Val params = Nil;
  u32 pos = NodePos(prefix, p->mem);
  InitLexer(&p->lex, p->lex.source, pos);

  if (!MatchToken(TokenLParen, &p->lex)) return ParseError("Expected \"(\"", p);

  while (!MatchToken(TokenRParen, &p->lex)) {
    result = ParseID(p);
    if (!result.ok) return result;
    params = Pair(result.value, params, p->mem);
  }

  Assert(MatchToken(TokenArrow, &p->lex));
  SkipNewlines(&p->lex);

  result = ParseExpr(PrecLambda, p);
  if (!result.ok) return result;

  return ParseOk(MakeNode(SymArrow, pos, Pair(ReverseList(params, p->mem), result.value, p->mem), p->mem));
}

static Result ParseLeftAssoc(Val prefix, Parser *p)
{
  Result result;
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  u32 pos = token.lexeme - p->lex.source;
  Val op = TokenSym(token.type);
  SkipNewlines(&p->lex);
  result = ParseExpr(prec+1, p);
  if (!result.ok) return result;

  return ParseOk(MakeNode(op, pos, Pair(prefix, Pair(result.value, Nil, p->mem), p->mem), p->mem));
}

static Result ParseRightAssoc(Val prefix, Parser *p)
{
  Result result;
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  u32 pos = token.lexeme - p->lex.source;
  Val op = TokenSym(token.type);
  SkipNewlines(&p->lex);
  result = ParseExpr(prec, p);
  if (!result.ok) return result;

  return ParseOk(MakeNode(op, pos, Pair(prefix, Pair(result.value, Nil, p->mem), p->mem), p->mem));
}

static Result ParseUnary(Parser *p)
{
  Result result;
  Token token = NextToken(&p->lex);
  u32 pos = token.lexeme - p->lex.source;
  Val op = TokenSym(token.type);
  result = ParseExpr(PrecUnary, p);
  if (!result.ok) return result;

  return ParseOk(MakeNode(op, pos, Pair(result.value, Nil, p->mem), p->mem));
}

static Result ParseGroup(Parser *p)
{
  Result result;
  u32 pos = p->lex.token.lexeme - p->lex.source;
  Val expr = Nil;
  Assert(MatchToken(TokenLParen, &p->lex));

  SkipNewlines(&p->lex);
  while (!MatchToken(TokenRParen, &p->lex)) {
    result = ParseExpr(PrecExpr, p);
    if (!result.ok) return result;
    expr = Pair(result.value, expr, p->mem);
    SkipNewlines(&p->lex);
  }
  return ParseOk(MakeNode(SymLParen, pos, ReverseList(expr, p->mem), p->mem));
}

static Result ParseDo(Parser *p)
{
  Result result;
  Val stmts = Nil;
  u32 pos = p->lex.token.lexeme - p->lex.source;
  Assert(MatchToken(TokenDo, &p->lex));

  SkipNewlines(&p->lex);
  while (!MatchToken(TokenEnd, &p->lex)) {
    result = ParseStmt(p);
    if (!result.ok) return result;
    stmts = Pair(result.value, stmts, p->mem);
    SkipNewlines(&p->lex);
  }

  if (Tail(stmts, p->mem) == Nil) return ParseOk(Head(stmts, p->mem));
  return ParseOk(MakeNode(SymDo, pos, ReverseList(stmts, p->mem), p->mem));
}

static Result ParseIf(Parser *p)
{
  Result result;
  Val pred, cons = Nil, alt = Nil;
  u32 pos = p->lex.token.lexeme - p->lex.source;
  u32 else_pos;

  Assert(MatchToken(TokenIf, &p->lex));

  result = ParseExpr(PrecExpr, p);
  if (!result.ok) return result;
  pred = result.value;

  if (!MatchToken(TokenDo, &p->lex)) return ParseError("Expected \"do\"", p);
  SkipNewlines(&p->lex);
  while (p->lex.token.type != TokenElse && p->lex.token.type != TokenEnd) {
    result = ParseStmt(p);
    if (!result.ok) return result;
    cons = Pair(result.value, cons, p->mem);

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
      result = ParseStmt(p);
      if (!result.ok) return result;
      alt = Pair(result.value, alt, p->mem);

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

  if (!MatchToken(TokenEnd, &p->lex)) return ParseError("Expected \"end\"", p);

  return ParseOk(MakeNode(SymIf, pos,
    Pair(pred,
    Pair(cons,
    Pair(alt, Nil, p->mem), p->mem), p->mem), p->mem));
}

static Result ParseClauses(Parser *p)
{
  Result result;
  u32 pos = p->lex.token.lexeme - p->lex.source;

  if (MatchToken(TokenEnd, &p->lex)) {
    return ParseOk(MakeNode(SymColon, pos, Nil, p->mem));
  } else {
    Val pred, cons;

    result = ParseExpr(PrecLogic, p);
    if (!result.ok) return result;
    pred = result.value;

    if (!MatchToken(TokenArrow, &p->lex)) return ParseError("Expected \"->\"", p);
    SkipNewlines(&p->lex);

    result = ParseCall(p);
    if (!result.ok) return result;
    cons = result.value;

    SkipNewlines(&p->lex);
    result = ParseClauses(p);
    if (!result.ok) return result;
    return ParseOk(MakeNode(SymIf, pos,
      Pair(pred,
      Pair(cons,
      Pair(result.value, Nil, p->mem), p->mem), p->mem), p->mem));
  }
}

static Result ParseCond(Parser *p)
{
  Assert(MatchToken(TokenCond, &p->lex));
  if (!MatchToken(TokenDo, &p->lex)) return ParseError("Expected \"do\"", p);
  SkipNewlines(&p->lex);
  return ParseClauses(p);
}

static Result ParseList(Parser *p)
{
  Result result;
  Val items = Nil;
  u32 pos = p->lex.token.lexeme - p->lex.source;

  Assert(MatchToken(TokenLBracket, &p->lex));
  while (!MatchToken(TokenRBracket, &p->lex)) {
    result = ParseExpr(PrecExpr, p);
    if (!result.ok) return result;
    items = Pair(result.value, items, p->mem);
  }

  return ParseOk(MakeNode(SymLBracket, pos, items, p->mem));
}

static Result ParseTuple(Parser *p)
{
  Result result;
  Val items = Nil;
  u32 pos = p->lex.token.lexeme - p->lex.source;

  Assert(MatchToken(TokenLBrace, &p->lex));
  while (!MatchToken(TokenRBrace, &p->lex)) {
    result = ParseExpr(PrecExpr, p);
    if (!result.ok) return result;

    items = Pair(result.value, items, p->mem);
  }

  return ParseOk(MakeNode(SymLBrace, pos, items, p->mem));
}

static Result ParseID(Parser *p)
{
  Token token = NextToken(&p->lex);
  if (token.type != TokenID) return ParseError("Expected identifier", p);
  return ParseOk(MakeSymbol(token.lexeme, token.length, p->symbols));
}

static Result ParseVar(Parser *p)
{
  u32 pos = p->lex.token.lexeme - p->lex.source;
  Result result = ParseID(p);
  if (!result.ok) return result;
  return ParseOk(MakeNode(SymID, pos, result.value, p->mem));
}

static Result ParseNum(Parser *p)
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
    return ParseOk(MakeNode(SymNum, pos, FloatVal(num), p->mem));
  } else {
    return ParseOk(MakeNode(SymNum, pos, IntVal(whole), p->mem));
  }
}

static Result ParseString(Parser *p)
{
  Token token = NextToken(&p->lex);
  u32 pos = token.lexeme - p->lex.source;
  Val symbol = MakeSymbol(token.lexeme + 1, token.length - 2, p->symbols);
  return ParseOk(MakeNode(SymString, pos, symbol, p->mem));
}

static Result ParseSymbol(Parser *p)
{
  Result result;
  u32 pos = p->lex.token.lexeme - p->lex.source;
  Assert(MatchToken(TokenColon, &p->lex));
  result = ParseID(p);
  if (!result.ok) return result;

  return ParseOk(MakeNode(SymColon, pos, result.value, p->mem));
}

static Result ParseLiteral(Parser *p)
{
  Token token = NextToken(&p->lex);
  u32 pos = token.lexeme - p->lex.source;

  switch (token.type) {
  case TokenTrue:   return ParseOk(MakeNode(SymColon, pos, True, p->mem));
  case TokenFalse:  return ParseOk(MakeNode(SymColon, pos, False, p->mem));
  case TokenNil:    return ParseOk(MakeNode(SymColon, pos, Nil, p->mem));
  default:          return ParseError("Unknown literal", p);
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

Val NodeType(Val node, Mem *mem)
{
  return TupleGet(node, 0, mem);
}

u32 NodePos(Val node, Mem *mem)
{
  return RawInt(TupleGet(node, 1, mem));
}

Val NodeExpr(Val node, Mem *mem)
{
  return TupleGet(node, 2, mem);
}

static Result ParseError(char *message, Parser *p)
{
  return ErrorResult(message, p->filename, p->lex.token.lexeme - p->lex.source);
}


#ifdef DEBUG
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
  Val tag = NodeType(node, mem);
  Val expr = NodeExpr(node, mem);

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
#endif
