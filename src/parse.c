#include "parse.h"
#include "lex.h"

typedef Val (*PrefixFn)(Lexer *lex, Mem *mem);
typedef Val (*InfixFn)(Val lhs, Lexer *lex, Mem *mem);

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
  PrecExponent,
  PrecUnary,
  PrecAccess
} Precedence;

typedef struct {
  PrefixFn prefix;
  InfixFn infix;
  Precedence precedence;
} ParseRule;

static Val ParseNum(Lexer *lex, Mem *mem);
static Val ParseString(Lexer *lex, Mem *mem);
static Val ParseID(Lexer *lex, Mem *mem);
static Val ParseLiteral(Lexer *lex, Mem *mem);
static Val ParseSymbol(Lexer *lex, Mem *mem);
static Val ParseParens(Lexer *lex, Mem *mem);
static Val ParseUnary(Lexer *lex, Mem *mem);
static Val ParseLet(Lexer *lex, Mem *mem);
static Val ParseDef(Lexer *lex, Mem *mem);
static Val ParseDo(Lexer *lex, Mem *mem);
static Val ParseIf(Lexer *lex, Mem *mem);
static Val ParseCond(Lexer *lex, Mem *mem);
static Val ParseList(Lexer *lex, Mem *mem);
static Val ParseTuple(Lexer *lex, Mem *mem);
static Val ParseMap(Lexer *lex, Mem *mem);
static Val ParseImport(Lexer *lex, Mem *mem);
static Val ParseAccess(Val lhs, Lexer *lex, Mem *mem);
static Val ParseLeftAssoc(Val lhs, Lexer *lex, Mem *mem);
static Val ParseRightAssoc(Val lhs, Lexer *lex, Mem *mem);

static ParseRule rules[] = {
  [TokenNum]          = {&ParseNum, NULL, PrecNone},
  [TokenID]           = {&ParseID, NULL, PrecNone},
  [TokenTrue]         = {&ParseLiteral, NULL, PrecNone},
  [TokenFalse]        = {&ParseLiteral, NULL, PrecNone},
  [TokenNil]          = {&ParseLiteral, NULL, PrecNone},
  [TokenColon]        = {&ParseSymbol, NULL, PrecNone},
  [TokenString]       = {&ParseString, NULL, PrecNone},
  [TokenLParen]       = {&ParseParens, NULL, PrecNone},
  [TokenNot]          = {&ParseUnary, NULL, PrecNone},
  [TokenHash]         = {&ParseUnary, NULL, PrecNone},
  [TokenPlus]         = {NULL, &ParseLeftAssoc, PrecSum},
  [TokenMinus]        = {&ParseUnary, &ParseLeftAssoc, PrecSum},
  [TokenCaret]        = {NULL, &ParseLeftAssoc, PrecExponent},
  [TokenStar]         = {NULL, &ParseLeftAssoc, PrecProduct},
  [TokenSlash]        = {NULL, &ParseLeftAssoc, PrecProduct},
  [TokenEqualEqual]   = {NULL, &ParseLeftAssoc, PrecEqual},
  [TokenNotEqual]     = {NULL, &ParseLeftAssoc, PrecEqual},
  [TokenGreater]      = {NULL, &ParseLeftAssoc, PrecCompare},
  [TokenGreaterEqual] = {NULL, &ParseLeftAssoc, PrecCompare},
  [TokenLess]         = {NULL, &ParseLeftAssoc, PrecCompare},
  [TokenLessEqual]    = {NULL, &ParseLeftAssoc, PrecCompare},
  [TokenIn]           = {NULL, &ParseLeftAssoc, PrecMember},
  [TokenPipe]         = {NULL, &ParseRightAssoc, PrecPair},
  [TokenAnd]          = {NULL, &ParseLeftAssoc, PrecLogic},
  [TokenOr]           = {NULL, &ParseLeftAssoc, PrecLogic},
  [TokenArrow]        = {NULL, &ParseLeftAssoc, PrecLambda},
  [TokenDot]          = {NULL, &ParseAccess, PrecAccess},
  [TokenLet]          = {&ParseLet, NULL, PrecNone},
  [TokenDef]          = {&ParseDef, NULL, PrecNone},
  [TokenDo]           = {&ParseDo, NULL, PrecNone},
  [TokenIf]           = {&ParseIf, NULL, PrecNone},
  [TokenCond]         = {&ParseCond, NULL, PrecNone},
  [TokenLBracket]     = {&ParseList, NULL, PrecNone},
  [TokenHashBracket]  = {&ParseTuple, NULL, PrecNone},
  [TokenLBrace]       = {&ParseMap, NULL, PrecNone},
  [TokenEOF]          = {NULL, NULL, PrecNone},
  [TokenComma]        = {NULL, NULL, PrecNone},
  [TokenEqual]        = {NULL, NULL, PrecNone},
  [TokenRParen]       = {NULL, NULL, PrecNone},
  [TokenEnd]          = {NULL, NULL, PrecNone},
  [TokenElse]         = {NULL, NULL, PrecNone},
  [TokenRBracket]     = {NULL, NULL, PrecNone},
  [TokenRBrace]       = {NULL, NULL, PrecNone},
  [TokenNewline]      = {NULL, NULL, PrecNone},
  [TokenImport]       = {&ParseImport, NULL, PrecNone},
};

#define GetRule(lex)    rules[PeekToken(lex).type]

static Val PartialParse(Mem *mem)
{
  return Pair(MakeSymbol("error", mem), MakeSymbol("partial", mem), mem);
}

static Val TokenVal(Token token, Lexer *lex, Mem *mem)
{
  u32 pos = token.lexeme - lex->text;
  return
    Pair(IntVal(token.type),
    Pair(IntVal(pos),
    Pair(IntVal(token.length),
    Pair(IntVal(token.line),
    Pair(IntVal(token.col), nil, mem), mem), mem), mem), mem);
}

static Val ParseError(char *message, Token token, Lexer *lex, Mem *mem)
{
  return
    Pair(MakeSymbol("error", mem),
    Pair(MakeSymbol("parse", mem),
    Pair(BinaryFrom(message, mem),
    Pair(TokenVal(token, lex, mem),
    Pair(BinaryFrom(lex->text, mem), nil, mem), mem), mem), mem), mem);
}

static bool MatchToken(TokenType type, Lexer *lex)
{
  if (PeekToken(lex).type == type) {
    NextToken(lex);
    return true;
  }
  return false;
}

static bool ExpectToken(TokenType type, Lexer *lex)
{
  if (!MatchToken(type, lex)) {
    return false;
  }
  return true;
}

static void SkipNewlines(Lexer *lex)
{
  while (MatchToken(TokenNewline, lex)) ;
}

Val ParseExpr(Precedence prec, Lexer *lex, Mem *mem)
{
  if (AtEnd(lex)) return PartialParse(mem);

  ParseRule rule = GetRule(lex);
  if (rule.prefix == NULL) return ParseError("Expected expression", lex->token, lex, mem);

  Val expr = rule.prefix(lex, mem);
  if (IsTagged(expr, "error", mem)) return expr;

  rule = GetRule(lex);
  while (rule.precedence >= prec) {
    expr = rule.infix(expr, lex, mem);
    if (IsTagged(expr, "error", mem)) return expr;
    rule = GetRule(lex);
  }

  return expr;
}

static Val ParseID(Lexer *lex, Mem *mem)
{
  if (AtEnd(lex)) return PartialParse(mem);

  Token token = NextToken(lex);
  if (token.type != TokenID) return ParseError("Expected identifier", token, lex, mem);
  return MakeSymbolFrom(token.lexeme, token.length, mem);
}

static u32 ParseDigit(char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
  if (c >= 'a' && c <= 'z') return c - 'a' + 10;
  return 0;
}

static u32 ParseInt(char *lexeme, u32 length, u32 base)
{
  u32 num = 0;
  for (u32 i = 0; i < length; i++) {
    num *= base;
    u32 digit = ParseDigit(lexeme[i]);
    num += digit;
  }

  return num;
}

static Val ParseNum(Lexer *lex, Mem *mem)
{
  Token token = NextToken(lex);
  Assert(token.type == TokenNum);

  if (token.length > 1 && token.lexeme[1] == 'x') {
    return IntVal(ParseInt(token.lexeme + 2, token.length - 2, 16));
  }

  u32 decimal = 0;
  while (decimal < token.length && token.lexeme[decimal] != '.') decimal++;

  u32 whole = ParseInt(token.lexeme, decimal, 10);

  if (decimal < token.length) {
    decimal++;
    for (u32 i = decimal; i < token.length; i++) {
      // float
      float frac = 0;
      float place = 0.1;

      for (u32 j = i; j < token.length; j++) {
        u32 digit = token.lexeme[j] - '0';
        frac += digit * place;
        place /= 10;
      }

      return NumVal(whole + frac);
    }
  }

  return IntVal(whole);
}

static Val ParseString(Lexer *lex, Mem *mem)
{
  Token token = NextToken(lex);
  Assert(token.type == TokenString);

  char str[token.length+1];
  u32 len = 0;
  for (u32 i = 0, j = 0; i < token.length; i++, j++) {
    if (token.lexeme[i] == '\\') i++;
    str[j] = token.lexeme[i];
    len++;
  }
  str[len] = '\0';

  Val sym = MakeSymbolFrom(str, len, mem);
  return Pair(MakeSymbol("\"", mem), sym, mem);
}

static Val ParseLiteral(Lexer *lex, Mem *mem)
{
  Token token = NextToken(lex);
  switch (token.type) {
  case TokenTrue:
    return MakeSymbol("true", mem);
  case TokenFalse:
    return MakeSymbol("false", mem);
  case TokenNil:
    return MakeSymbol("nil", mem);
  default:
    return ParseError("Unknown literal", token, lex, mem);
  }
}

static Val ParseSymbol(Lexer *lex, Mem *mem)
{
  if (!ExpectToken(TokenColon, lex)) return ParseError("Expected \":\"", lex->token, lex, mem);

  Val id = ParseID(lex, mem);
  if (IsTagged(id, "error", mem)) return id;
  return Pair(MakeSymbol(":", mem), id, mem);
}

static Val ParseParens(Lexer *lex, Mem *mem)
{
  if (!ExpectToken(TokenLParen, lex)) return ParseError("Expected \"(\"", lex->token, lex, mem);

  SkipNewlines(lex);
  Val args = nil;
  while (!MatchToken(TokenRParen, lex)) {
    Val arg = ParseExpr(PrecExpr, lex, mem);
    if (IsTagged(arg, "error", mem)) return arg;

    args = Pair(arg, args, mem);
    SkipNewlines(lex);
  }

  return ReverseList(args, mem);
}

static Val ParseUnary(Lexer *lex, Mem *mem)
{
  Token token = NextToken(lex);
  Val op = MakeSymbolFrom(token.lexeme, token.length, mem);
  Val rhs = ParseExpr(PrecUnary, lex, mem);
  if (IsTagged(rhs, "error", mem)) return rhs;
  return Pair(op, Pair(rhs, nil, mem), mem);
}

static Val ParseAssign(Lexer *lex, Mem *mem)
{
  Val id = ParseID(lex, mem);
  if (IsTagged(id, "error", mem)) return id;

  SkipNewlines(lex);
  if (AtEnd(lex)) return PartialParse(mem);

  if (!ExpectToken(TokenEqual, lex)) return ParseError("Expected \"=\"", lex->token, lex, mem);
  SkipNewlines(lex);

  Val value = ParseExpr(PrecExpr, lex, mem);
  if (IsTagged(value, "error", mem)) return value;
  return Pair(id, value, mem);
}

static Val ParseLet(Lexer *lex, Mem *mem)
{
  if (!ExpectToken(TokenLet, lex)) return ParseError("Expected \"let\"", lex->token, lex, mem);
  SkipNewlines(lex);
  if (AtEnd(lex)) return PartialParse(mem);

  Val assign = ParseAssign(lex, mem);
  if (IsTagged(assign, "error", mem)) return assign;
  Val assigns = Pair(assign, nil, mem);

  while (MatchToken(TokenComma, lex)) {
    SkipNewlines(lex);
    if (AtEnd(lex)) return PartialParse(mem);

    Val assign = ParseAssign(lex, mem);
    if (IsTagged(assign, "error", mem)) return assign;
    assigns = Pair(assign, assigns, mem);
  }

  return Pair(MakeSymbol("let", mem), ReverseList(assigns, mem), mem);
}

static Val ParseDef(Lexer *lex, Mem *mem)
{
  if (!ExpectToken(TokenDef, lex)) return ParseError("Expected \"def\"", lex->token, lex, mem);
  if (AtEnd(lex)) return PartialParse(mem);
  if (!ExpectToken(TokenLParen, lex)) return ParseError("Expected \"(\"", lex->token, lex, mem);

  Val id = ParseID(lex, mem);
  if (IsTagged(id, "error", mem)) return id;

  Val params = nil;
  while (!MatchToken(TokenRParen, lex)) {
    Val param = ParseID(lex, mem);
    if (IsTagged(param, "error", mem)) return param;
    params = Pair(param, params, mem);
  }

  Val body = ParseExpr(PrecExpr, lex, mem);
  if (IsTagged(body, "error", mem)) return body;

  Val lambda =
    Pair(MakeSymbol("->", mem),
    Pair(params,
    Pair(body, nil, mem), mem), mem);

  return Pair(MakeSymbol("let", mem), Pair(Pair(id, lambda, mem), nil, mem), mem);
}

static Val ParseStmt(Lexer *lex, Mem *mem)
{
  Val args = nil;
  while (PeekToken(lex).type != TokenNewline && !AtEnd(lex)) {
    Val arg = ParseExpr(PrecExpr, lex, mem);
    if (IsTagged(arg, "error", mem)) return arg;

    args = Pair(arg, args, mem);
  }

  if (ListLength(args, mem) == 1) return Head(args, mem);
  return ReverseList(args, mem);
}

static Val ParseDo(Lexer *lex, Mem *mem)
{
  if (!ExpectToken(TokenDo, lex)) return ParseError("Expected \"do\"", lex->token, lex, mem);
  SkipNewlines(lex);

  Val stmts = nil;
  while (!MatchToken(TokenEnd, lex)) {
    if (AtEnd(lex)) return PartialParse(mem);

    Val stmt = ParseStmt(lex, mem);
    if (IsTagged(stmt, "error", mem)) return stmt;
    stmts = Pair(stmt, stmts, mem);
    SkipNewlines(lex);
  }

  if (ListLength(stmts, mem) == 1) return Head(stmts, mem);

  return Pair(MakeSymbol("do", mem), ReverseList(stmts, mem), mem);
}

static Val ParseIf(Lexer *lex, Mem *mem)
{
  if (!ExpectToken(TokenIf, lex)) return ParseError("Expected \"if\"", lex->token, lex, mem);

  Val predicate = ParseExpr(PrecExpr, lex, mem);
  if (IsTagged(predicate, "error", mem)) return predicate;

  if (!ExpectToken(TokenDo, lex)) return ParseError("Expected \"do\"", lex->token, lex, mem);
  SkipNewlines(lex);

  Val true_block = nil;
  while (!MatchToken(TokenEnd, lex) && PeekToken(lex).type != TokenElse) {
    if (AtEnd(lex)) return PartialParse(mem);

    Val stmt = ParseStmt(lex, mem);
    if (IsTagged(stmt, "error", mem)) return stmt;

    true_block = Pair(stmt, true_block, mem);
    SkipNewlines(lex);
  }

  if (ListLength(true_block, mem) == 1) {
    true_block = Head(true_block, mem);
  } else {
    true_block = ReverseList(true_block, mem);
  }

  Val false_block = nil;
  if (MatchToken(TokenElse, lex)) {
    SkipNewlines(lex);

    while (!MatchToken(TokenEnd, lex)) {
      if (AtEnd(lex)) return PartialParse(mem);

      Val stmt = ParseStmt(lex, mem);
      if (IsTagged(stmt, "error", mem)) return stmt;

      false_block = Pair(stmt, false_block, mem);
      SkipNewlines(lex);
    }
  }

  if (IsNil(false_block)) {
    false_block = MakeSymbol("nil", mem);
  } else if (ListLength(false_block, mem) == 1) {
    false_block = Head(false_block, mem);
  } else {
    false_block = ReverseList(false_block, mem);
  }

  return
    Pair(MakeSymbol("if", mem),
    Pair(predicate,
    Pair(true_block,
    Pair(false_block, nil, mem), mem), mem), mem);
}

static Val ParseClauses(Lexer *lex, Mem *mem)
{
  Val predicate = ParseExpr(PrecLogic, lex, mem);
  if (IsTagged(predicate, "error", mem)) return predicate;

  SkipNewlines(lex);
  if (!ExpectToken(TokenArrow, lex)) return ParseError("Expected \"->\"", lex->token, lex, mem);

  SkipNewlines(lex);
  Val consequent = ParseStmt(lex, mem);
  if (IsTagged(consequent, "error", mem)) return consequent;

  SkipNewlines(lex);
  Val alternative = nil;
  if (!MatchToken(TokenEnd, lex)) {
    alternative = ParseClauses(lex, mem);
    if (IsTagged(alternative, "error", mem)) return alternative;
  }

  return
    Pair(MakeSymbol("if", mem),
    Pair(predicate,
    Pair(consequent,
    Pair(alternative, nil, mem), mem), mem), mem);
}

static Val ParseCond(Lexer *lex, Mem  *mem)
{
  if (!ExpectToken(TokenCond, lex)) return ParseError("Expected \"cond\"", lex->token, lex, mem);

  SkipNewlines(lex);
  if (AtEnd(lex)) return PartialParse(mem);

  if (!ExpectToken(TokenDo, lex)) return ParseError("Expected \"do\"", lex->token, lex, mem);

  SkipNewlines(lex);
  return ParseClauses(lex, mem);
}

static Val ParseList(Lexer *lex, Mem *mem)
{
  if (!ExpectToken(TokenLBracket, lex)) return ParseError("Expected \"[\"", lex->token, lex, mem);
  SkipNewlines(lex);

  Val items = nil;
  while (!MatchToken(TokenRBracket, lex)) {
    Val item = ParseExpr(PrecExpr, lex, mem);
    if (IsTagged(item, "error", mem)) return item;

    items = Pair(item, items, mem);
    MatchToken(TokenComma, lex);
    SkipNewlines(lex);
  }

  return Pair(MakeSymbol("[", mem), ReverseList(items, mem), mem);
}

static Val ParseTuple(Lexer *lex, Mem *mem)
{
  if (!ExpectToken(TokenHashBracket, lex)) return ParseError("Expected \"#[\"", lex->token, lex, mem);

  SkipNewlines(lex);
  Val items = nil;
  while (!MatchToken(TokenRBracket, lex)) {
    Val item = ParseExpr(PrecExpr, lex, mem);
    if (IsTagged(item, "error", mem)) return item;

    items = Pair(item, items, mem);
    MatchToken(TokenComma, lex);
    SkipNewlines(lex);
  }

  return Pair(MakeSymbol("#[", mem), ReverseList(items, mem), mem);
}

static Val ParseMap(Lexer *lex, Mem *mem)
{
  if (!ExpectToken(TokenLBrace, lex)) return ParseError("Expected \"{\"", lex->token, lex, mem);

  SkipNewlines(lex);
  Val items = nil;
  while (!MatchToken(TokenRBrace, lex)) {
    Val key = ParseID(lex, mem);

    if (!ExpectToken(TokenColon, lex)) return ParseError("Expected \":\"", lex->token, lex, mem);

    SkipNewlines(lex);
    Val val = ParseExpr(PrecExpr, lex, mem);
    if (IsTagged(val, "error", mem)) return val;

    Val item = Pair(key, val, mem);
    items = Pair(item, items, mem);
    MatchToken(TokenComma, lex);
    SkipNewlines(lex);
  }

  return Pair(MakeSymbol("{", mem), ReverseList(items, mem), mem);
}

static Val ParseImport(Lexer *lex, Mem *mem)
{
  if (!ExpectToken(TokenImport, lex)) return ParseError("Expected \"import\"", lex->token, lex, mem);
  if (AtEnd(lex)) return PartialParse(mem);

  Val name = ParseString(lex, mem);
  if (MatchToken(TokenAs, lex)) {
    Val alias = ParseID(lex, mem);
    if (IsTagged(alias, "error", mem)) return alias;

    return
      Pair(MakeSymbol("import", mem),
      Pair(Tail(name, mem),
      Pair(alias, nil, mem), mem), mem);
  } else {
    return
      Pair(MakeSymbol("import", mem),
      Pair(Tail(name, mem), nil, mem), mem);
  }
}

static Val ParseAccess(Val lhs, Lexer *lex, Mem *mem)
{
  if (!ExpectToken(TokenDot, lex)) return ParseError("Expected \".\"", lex->token, lex, mem);

  Val id = ParseID(lex, mem);
  if (IsTagged(id, "error", mem)) return id;

  Val key = Pair(MakeSymbol(":", mem), id, mem);
  return
    Pair(MakeSymbol(".", mem),
    Pair(lhs,
    Pair(key, nil, mem), mem), mem);
}

static Val ParseLeftAssoc(Val lhs, Lexer *lex, Mem *mem)
{
  Precedence prec = GetRule(lex).precedence;
  Token token = NextToken(lex);
  Val op = MakeSymbolFrom(token.lexeme, token.length, mem);
  SkipNewlines(lex);
  Val rhs = ParseExpr(prec + 1, lex, mem);
  if (IsTagged(rhs, "error", mem)) return rhs;

  return Pair(op, Pair(lhs, Pair(rhs, nil, mem), mem), mem);
}

static Val ParseRightAssoc(Val lhs, Lexer *lex, Mem *mem)
{
  Precedence prec = GetRule(lex).precedence;
  Token token = NextToken(lex);
  Val op = MakeSymbolFrom(token.lexeme, token.length, mem);
  SkipNewlines(lex);
  Val rhs = ParseExpr(prec, lex, mem);
  if (IsTagged(rhs, "error", mem)) return rhs;

  return Pair(op, Pair(lhs, Pair(rhs, nil, mem), mem), mem);
}

Val Parse(char *source, Mem *mem)
{
  Lexer lex;
  InitLexer(&lex, source);
  SkipNewlines(&lex);

  Val stmts = nil;
  while (!AtEnd(&lex)) {
    Val stmt = ParseStmt(&lex, mem);
    if (IsTagged(stmt, "error", mem)) return stmt;

    stmts = Pair(stmt, stmts, mem);
    SkipNewlines(&lex);
  }

  if (ListLength(stmts, mem) < 2) {
    return Head(stmts, mem);
  }

  return Pair(MakeSymbol("do", mem), ReverseList(stmts, mem), mem);
}
