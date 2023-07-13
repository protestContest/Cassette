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
static Val ParsePrefix(Lexer *lex, Mem *mem);
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
  [TokenPlus]         = {NULL, &ParseLeftAssoc, PrecSum},
  [TokenMinus]        = {&ParsePrefix, &ParseLeftAssoc, PrecSum},
  [TokenNot]          = {&ParsePrefix, NULL, PrecNone},
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
  [TokenError]        = {NULL, NULL, PrecNone},
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

static void ParseError(Lexer *lex, Token token, char *message)
{
  PrintEscape(IOFGRed);
  Print("Parse error [");
  PrintInt(token.line);
  Print(":");
  PrintInt(token.col);
  Print("]: ");
  Print(message);
  Print("\n");

  u32 token_pos = token.lexeme - lex->text;
  PrintSourceContext(lex->text, token.line, 3, token_pos, token.length);

  PrintEscape(IOFGReset);
  Print("\n");
}

static void PrintParser(Lexer *lex)
{
  PrintToken(lex->token);
  Print(" [");
  PrintInt(lex->token.line);
  Print(":");
  PrintInt(lex->token.col);
  Print("]\n");

  u32 pos = lex->token.lexeme - lex->text;
  PrintSourceContext(lex->text, lex->token.line, 3, pos, lex->token.length);
  Print("\n");
}

static bool MatchToken(Lexer *lex, TokenType type)
{
  if (PeekToken(lex).type == type) {
    NextToken(lex);
    return true;
  }
  return false;
}

static bool ExpectToken(Lexer *lex, TokenType type)
{
  if (!MatchToken(lex, type)) {
    ParseError(lex, lex->token, "Unexpected token");
    return false;
  }
  return true;
}

static void SkipNewlines(Lexer *lex)
{
  while (MatchToken(lex, TokenNewline)) ;
}

Val ParseExpr(Precedence prec, Lexer *lex, Mem *mem)
{
  // Print("Parsing ");
  // PrintToken(PeekToken(lex));
  // Print(" at precedence ");
  // PrintInt(prec);
  // Print("\n");
  // PrintParser(lex);

  ParseRule rule = GetRule(lex);
  if (rule.prefix == NULL) {
    ParseError(lex, lex->token, "Expected expr");
    return Pair(MakeSymbol("error", mem), MakeSymbol("parse", mem), mem);
  }

  Val expr = rule.prefix(lex, mem);
  if (IsTagged(expr, "error", mem)) return expr;

  rule = GetRule(lex);
  // Print("  ");
  // PrintToken(PeekToken(lex));
  // Print(" ");
  // PrintInt(rule.precedence);
  // Print(" >= ");
  // PrintInt(prec);
  // Print("? ");
  while (rule.precedence >= prec) {
    // Print("ok\n");
    expr = rule.infix(expr, lex, mem);
    if (IsTagged(expr, "error", mem)) return expr;
    rule = GetRule(lex);
    // Print("  ");
    // PrintToken(PeekToken(lex));
    // Print(" ");
    // PrintInt(rule.precedence);
    // Print(" >= ");
    // PrintInt(prec);
    // Print("? ");
  }
  // Print("no\n");

  return expr;
}

static Val ParseID(Lexer *lex, Mem *mem)
{
  Token token = NextToken(lex);
  if (token.type != TokenID) {
    ParseError(lex, token, "Expected identifier");
    return Pair(MakeSymbol("error", mem), MakeSymbol("parse", mem), mem);
  }
  return MakeSymbolFrom(token.lexeme, token.length, mem);
}

static Val ParseNum(Lexer *lex, Mem *mem)
{
  Token token = NextToken(lex);

  Assert(token.type == TokenNum);

  u32 whole = 0;
  for (u32 i = 0; i < token.length; i++) {
    if (token.lexeme[i] == '.') {
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

    u32 digit = token.lexeme[i] - '0';
    whole *= 10;
    whole += digit;
  }

  return IntVal(whole);
}

static Val ParseString(Lexer *lex, Mem *mem)
{
  Token token = NextToken(lex);
  if (token.type != TokenString) {
    ParseError(lex, token, "Expected string");
    return Pair(MakeSymbol("error", mem), MakeSymbol("parse", mem), mem);
  }

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
    ParseError(lex, token, "Unknown literal");
    return Pair(MakeSymbol("error", mem), MakeSymbol("parse", mem), mem);
  }
}

static Val ParseSymbol(Lexer *lex, Mem *mem)
{
  ExpectToken(lex, TokenColon);
  Token token = NextToken(lex);
  Val id = MakeSymbolFrom(token.lexeme, token.length, mem);
  return Pair(MakeSymbol(":", mem), id, mem);
}

static Val ParseParens(Lexer *lex, Mem *mem)
{
  // Token paren = lex->token;
  ExpectToken(lex, TokenLParen);
  SkipNewlines(lex);

  Val args = nil;
  while (!MatchToken(lex, TokenRParen)) {
    if (AtEnd(lex)) {
      return Pair(MakeSymbol("error", mem), MakeSymbol("partial", mem), mem);
      // ParseError(lex, paren, "Unmatched parentheses");
      // return nil;
    }

    Val arg = ParseExpr(PrecExpr, lex, mem);
    if (IsTagged(arg, "error", mem)) return arg;

    args = Pair(arg, args, mem);
    SkipNewlines(lex);
  }

  return ReverseList(args, mem);
}

static Val ParsePrefix(Lexer *lex, Mem *mem)
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
  ExpectToken(lex, TokenEqual);
  Val value = ParseExpr(PrecExpr, lex, mem);
  if (IsTagged(value, "error", mem)) return value;
  return Pair(id, value, mem);
}

static Val ParseLet(Lexer *lex, Mem *mem)
{
  ExpectToken(lex, TokenLet);
  SkipNewlines(lex);
  Val assigns = Pair(ParseAssign(lex, mem), nil, mem);

  while (MatchToken(lex, TokenComma)) {
    SkipNewlines(lex);
    assigns = Pair(ParseAssign(lex, mem), assigns, mem);
  }

  return Pair(MakeSymbol("let", mem), ReverseList(assigns, mem), mem);
}

static Val ParseDef(Lexer *lex, Mem *mem)
{
  ExpectToken(lex, TokenDef);
  // Token paren = lex->token;
  ExpectToken(lex, TokenLParen);

  Val id = ParseID(lex, mem);

  Val params = nil;
  while (!MatchToken(lex, TokenRParen)) {
    if (PeekToken(lex).type == TokenEOF) {
      return Pair(MakeSymbol("error", mem), MakeSymbol("partial", mem), mem);
      // ParseError(lex, paren, "Unmatched parentheses");
      // return nil;
    }

    Val param = ParseID(lex, mem);
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
  ExpectToken(lex, TokenDo);
  SkipNewlines(lex);

  Val stmts = nil;

  while (!MatchToken(lex, TokenEnd)) {
    if (AtEnd(lex)) {
      return Pair(MakeSymbol("error", mem), MakeSymbol("partial", mem), mem);
    }

    Val stmt = ParseStmt(lex, mem);
    if (IsTagged(stmt, "error", mem)) return stmt;

    stmts = Pair(stmt, stmts, mem);

    SkipNewlines(lex);
  }

  if (ListLength(stmts, mem) == 1) {
    return Head(stmts, mem);
  }

  return Pair(MakeSymbol("do", mem), ReverseList(stmts, mem), mem);
}

static Val ParseIf(Lexer *lex, Mem *mem)
{
  // Token begin = lex->token;
  ExpectToken(lex, TokenIf);

  Val predicate = ParseExpr(PrecExpr, lex, mem);
  if (IsTagged(predicate, "error", mem)) return predicate;

  ExpectToken(lex, TokenDo);
  SkipNewlines(lex);

  Val true_block = nil;
  while (!MatchToken(lex, TokenEnd) && PeekToken(lex).type != TokenElse) {
    if (AtEnd(lex)) {
      return Pair(MakeSymbol("error", mem), MakeSymbol("partial", mem), mem);
      // ParseError(lex, begin, "Unterminated if-block");
      // return nil;
    }

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
  // begin = lex->token;
  if (MatchToken(lex, TokenElse)) {
    SkipNewlines(lex);

    while (!MatchToken(lex, TokenEnd)) {
      if (AtEnd(lex)) {
        return Pair(MakeSymbol("error", mem), MakeSymbol("partial", mem), mem);
        // ParseError(lex, begin, "Unterminated else-block");
        // return nil;
      }

      Val stmt = ParseStmt(lex, mem);
      if (IsTagged(stmt, "error", mem)) return stmt;
      false_block = Pair(stmt, false_block, mem);

      SkipNewlines(lex);
    }
  }
  if (ListLength(false_block, mem) == 1) {
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
  if (AtEnd(lex)) return PartialParse(mem);

  Val predicate = ParseExpr(PrecLogic, lex, mem);
  if (IsTagged(predicate, "error", mem)) return predicate;
  SkipNewlines(lex);
  ExpectToken(lex, TokenArrow);
  SkipNewlines(lex);

  Val consequent = ParseExpr(PrecExpr, lex, mem);
  if (IsTagged(consequent, "error", mem)) return consequent;
  SkipNewlines(lex);

  Val alternative = nil;
  if (!MatchToken(lex, TokenEnd)) {
    alternative = ParseClauses(lex, mem);
  }
  return
    Pair(MakeSymbol("if", mem),
    Pair(predicate,
    Pair(consequent,
    Pair(alternative, nil, mem), mem), mem), mem);
}

static Val ParseCond(Lexer *lex, Mem  *mem)
{
  ExpectToken(lex, TokenCond);
  SkipNewlines(lex);
  ExpectToken(lex, TokenDo);
  SkipNewlines(lex);

  return ParseClauses(lex, mem);
}

static Val ParseList(Lexer *lex, Mem *mem)
{
  ExpectToken(lex, TokenLBracket);
  SkipNewlines(lex);

  Val items = nil;
  while (!MatchToken(lex, TokenRBracket)) {
    if (AtEnd(lex)) return PartialParse(mem);

    Val item = ParseExpr(PrecExpr, lex, mem);
    if (IsTagged(item, "error", mem)) return item;

    items = Pair(item, items, mem);

    MatchToken(lex, TokenComma);
    SkipNewlines(lex);
  }

  return Pair(MakeSymbol("[", mem), ReverseList(items, mem), mem);
}

static Val ParseTuple(Lexer *lex, Mem *mem)
{
  // Token bracket = lex->token;
  ExpectToken(lex, TokenHashBracket);
  SkipNewlines(lex);

  Val items = nil;
  while (!MatchToken(lex, TokenRBracket)) {
    if (AtEnd(lex)) {
      return Pair(MakeSymbol("error", mem), MakeSymbol("partial", mem), mem);
      // ParseError(lex, bracket, "Unmatched bracket");
      // return nil;
    }

    Val item = ParseExpr(PrecExpr, lex, mem);
    if (IsTagged(item, "error", mem)) return item;
    items = Pair(item, items, mem);

    MatchToken(lex, TokenComma);
    SkipNewlines(lex);
  }

  return Pair(MakeSymbol("#[", mem), ReverseList(items, mem), mem);
}

static Val ParseMap(Lexer *lex, Mem *mem)
{
  // Token brace = lex->token;
  ExpectToken(lex, TokenLBrace);
  SkipNewlines(lex);

  Val items = nil;
  while (!MatchToken(lex, TokenRBrace)) {
    if (AtEnd(lex)) {
      return Pair(MakeSymbol("error", mem), MakeSymbol("partial", mem), mem);
      // ParseError(lex, brace, "Unmatched brace");
      // return nil;
    }

    Val key = ParseID(lex, mem);

    ExpectToken(lex, TokenColon);

    Val val = ParseExpr(PrecExpr, lex, mem);
    if (IsTagged(val, "error", mem)) return val;


    Val item = Pair(key, val, mem);
    items = Pair(item, items, mem);

    MatchToken(lex, TokenComma);
    SkipNewlines(lex);
  }

  return Pair(MakeSymbol("{", mem), ReverseList(items, mem), mem);
}

static Val ParseImport(Lexer *lex, Mem *mem)
{
  ExpectToken(lex, TokenImport);
  Val name = ParseString(lex, mem);
  if (MatchToken(lex, TokenAs)) {
    Val alias = ParseID(lex, mem);
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
  ExpectToken(lex, TokenDot);

  Token token = lex->token;
  ExpectToken(lex, TokenID);

  Val id = MakeSymbolFrom(token.lexeme, token.length, mem);
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
  Val rhs = ParseExpr(prec + 1, lex, mem);
  if (IsTagged(rhs, "error", mem)) return rhs;

  return Pair(op, Pair(lhs, Pair(rhs, nil, mem), mem), mem);
}

static Val ParseRightAssoc(Val lhs, Lexer *lex, Mem *mem)
{
  Precedence prec = GetRule(lex).precedence;
  Token token = NextToken(lex);
  Val op = MakeSymbolFrom(token.lexeme, token.length, mem);
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
    stmts = Pair(stmt, stmts, mem);
    SkipNewlines(&lex);
  }

  if (ListLength(stmts, mem) < 2) {
    return Head(stmts, mem);
  }

  return Pair(MakeSymbol("do", mem), ReverseList(stmts, mem), mem);
}
