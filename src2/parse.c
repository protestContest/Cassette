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
  PrecUnary
} Precedence;

typedef struct {
  PrefixFn prefix;
  InfixFn infix;
  Precedence precedence;
} ParseRule;

static Val ParseNum(Lexer *lex, Mem *mem);
static Val ParseVar(Lexer *lex, Mem *mem);
static Val ParseLiteral(Lexer *lex, Mem *mem);
static Val ParseSymbol(Lexer *lex, Mem *mem);
static Val ParseParens(Lexer *lex, Mem *mem);
static Val ParsePrefix(Lexer *lex, Mem *mem);
static Val ParseInfix(Val lhs, Lexer *lex, Mem *mem);

static ParseRule rules[] = {
  [TokenNum]          = {&ParseNum, NULL, PrecNone},
  [TokenID]           = {&ParseVar, NULL, PrecNone},
  [TokenTrue]         = {&ParseLiteral, NULL, PrecNone},
  [TokenFalse]        = {&ParseLiteral, NULL, PrecNone},
  [TokenNil]          = {&ParseLiteral, NULL, PrecNone},
  [TokenColon]        = {&ParseSymbol, NULL, PrecNone},
  [TokenLParen]       = {&ParseParens, NULL, PrecNone},
  [TokenPlus]         = {NULL, &ParseInfix, PrecSum},
  [TokenMinus]        = {&ParsePrefix, &ParseInfix, PrecSum},
  [TokenNot]          = {&ParsePrefix, NULL, PrecNone},
  [TokenStar]         = {NULL, &ParseInfix, PrecProduct},
  [TokenSlash]        = {NULL, &ParseInfix, PrecProduct},
  [TokenEqualEqual]   = {NULL, &ParseInfix, PrecEqual},
  [TokenNotEqual]     = {NULL, &ParseInfix, PrecEqual},
  [TokenGreater]      = {NULL, &ParseInfix, PrecCompare},
  [TokenGreaterEqual] = {NULL, &ParseInfix, PrecCompare},
  [TokenLess]         = {NULL, &ParseInfix, PrecCompare},
  [TokenLessEqual]    = {NULL, &ParseInfix, PrecCompare},
  [TokenIn]           = {NULL, &ParseInfix, PrecMember},
  [TokenPipe]         = {NULL, &ParseInfix, PrecPair},
  [TokenAnd]          = {NULL, &ParseInfix, PrecLogic},
  [TokenOr]           = {NULL, &ParseInfix, PrecLogic},
  [TokenEOF]          = {NULL, NULL, PrecNone},
  [TokenError]        = {NULL, NULL, PrecNone},
  [TokenLet]          = {NULL, NULL, PrecNone},
  [TokenComma]        = {NULL, NULL, PrecNone},
  [TokenEqual]        = {NULL, NULL, PrecNone},
  [TokenDef]          = {NULL, NULL, PrecNone},
  [TokenRParen]       = {NULL, NULL, PrecNone},
  [TokenArrow]        = {NULL, NULL, PrecNone},
  [TokenString]       = {NULL, NULL, PrecNone},
  [TokenDot]          = {NULL, NULL, PrecNone},
  [TokenDo]           = {NULL, NULL, PrecNone},
  [TokenEnd]          = {NULL, NULL, PrecNone},
  [TokenIf]           = {NULL, NULL, PrecNone},
  [TokenElse]         = {NULL, NULL, PrecNone},
  [TokenCond]         = {NULL, NULL, PrecNone},
  [TokenLBracket]     = {NULL, NULL, PrecNone},
  [TokenRBracket]     = {NULL, NULL, PrecNone},
  [TokenHashBracket]  = {NULL, NULL, PrecNone},
  [TokenLBrace]       = {NULL, NULL, PrecNone},
  [TokenRBrace]       = {NULL, NULL, PrecNone},
  [TokenNewline]      = {NULL, NULL, PrecNone},
  [TokenModule]       = {NULL, NULL, PrecNone},
};

#define GetRule(lex)    rules[PeekToken(lex).type]

static void ParseError(Lexer *lex, char *message)
{
  PrintEscape(IOFGRed);
  Print("Parse error [");
  PrintInt(lex->token.line);
  Print(":");
  PrintInt(lex->token.col);
  Print("]: ");
  Print(message);
  Print("\n");

  u32 token_pos = lex->token.lexeme - lex->text;
  PrintSourceContext(lex->text, lex->token.line, 3, token_pos, lex->token.length);

  PrintEscape(IOFGReset);
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
    ParseError(lex, "Unexpected token");
    return false;
  }
  return true;
}

Val ParseExpr(Precedence prec, Lexer *lex, Mem *mem)
{
  Print("Parsing ");
  PrintToken(PeekToken(lex));
  Print(" at precedence ");
  PrintInt(prec);
  Print("\n");

  ParseRule rule = GetRule(lex);
  if (rule.prefix == NULL) {
    ParseError(lex, "Expected expr");
    return nil;
  }

  Val expr = rule.prefix(lex, mem);
  if (IsNil(expr)) return nil;

  rule = GetRule(lex);
  Print("  ");
  PrintToken(PeekToken(lex));
  Print(" ");
  PrintInt(rule.precedence);
  Print(" >= ");
  PrintInt(prec);
  Print("? ");
  while (rule.precedence >= prec) {
    Print("ok\n");
    expr = rule.infix(expr, lex, mem);
    if (IsNil(expr)) return nil;
    rule = GetRule(lex);
    Print("  ");
    PrintToken(PeekToken(lex));
    Print(" ");
    PrintInt(rule.precedence);
    Print(" >= ");
    PrintInt(prec);
    Print("? ");
  }
  Print("no\n");

  return expr;
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

static Val ParseVar(Lexer *lex, Mem *mem)
{
  Token token = NextToken(lex);
  return MakeSymbol(token.lexeme, token.length, mem);
}

static Val ParseLiteral(Lexer *lex, Mem *mem)
{
  Token token = NextToken(lex);
  switch (token.type) {
  case TokenTrue:
    return MakeSymbol("true", 4, mem);
  case TokenFalse:
    return MakeSymbol("false", 5, mem);
  case TokenNil:
    return MakeSymbol("nil", 3, mem);
  default:
    ParseError(lex, "Unknown literal");
    return nil;
  }
}

static Val ParseSymbol(Lexer *lex, Mem *mem)
{
  ExpectToken(lex, TokenColon);
  Token token = NextToken(lex);
  Val id = MakeSymbol(token.lexeme, token.length, mem);
  return Pair(MakeSymbol(":", 1, mem), id, mem);
}

static Val ParseParens(Lexer *lex, Mem *mem)
{
  ExpectToken(lex, TokenLParen);

  Val params = nil;
  while (PeekToken(lex).type == TokenID) {
    Val param = ParseVar(lex, mem);
    params = Pair(param, params, mem);
  }

  if (MatchToken(lex, TokenRParen)) {
    if (MatchToken(lex, TokenArrow)) {
      Val rhs = ParseExpr(PrecExpr, lex, mem);
      if (IsNil(rhs)) return nil;

      return
        Pair(MakeSymbol("->", 2, mem),
        Pair(params,
        Pair(rhs, nil, mem), mem), mem);
    } else {
      return params;
    }
  }

  // not a lambda, params are actually arguments
  Val args = params;
  while (PeekToken(lex).type != TokenRParen) {
    if (AtEnd(lex)) {
      ParseError(lex, "Unmatched parentheses");
      return nil;
    }

    Val arg = ParseExpr(PrecExpr, lex, mem);
    if (IsNil(arg)) return nil;

    args = Pair(arg, args, mem);
  }
  return args;
}

static Val ParsePrefix(Lexer *lex, Mem *mem)
{
  Precedence prec = GetRule(lex).precedence;
  Token token = NextToken(lex);
  Val op = MakeSymbol(token.lexeme, token.length, mem);
  Val rhs = ParseExpr(prec, lex, mem);
  if (IsNil(rhs)) return nil;
  return Pair(op, Pair(rhs, nil, mem), mem);
}

static Val ParseInfix(Val lhs, Lexer *lex, Mem *mem)
{
  Precedence prec = GetRule(lex).precedence;
  Token token = NextToken(lex);
  Val op = MakeSymbol(token.lexeme, token.length, mem);
  Val rhs = ParseExpr(prec + 1, lex, mem);
  if (IsNil(rhs)) return nil;

  return Pair(op, Pair(lhs, Pair(rhs, nil, mem), mem), mem);
}

Val Parse(char *source, Mem *mem)
{
  Lexer lex;
  InitLexer(&lex, source);
  return ParseExpr(PrecExpr, &lex, mem);
}
