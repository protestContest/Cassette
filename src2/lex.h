#pragma once
#include "mem.h"
#include "source.h"

typedef enum {
  TokenEOF          = 0,
  TokenError        = 1,
  TokenLet          = 2,
  TokenComma        = 3,
  TokenEqual        = 4,
  TokenDef          = 5,
  TokenLParen       = 6,
  TokenRParen       = 7,
  TokenID           = 8,
  TokenArrow        = 9,
  TokenAnd          = 10,
  TokenOr           = 11,
  TokenEqualEqual   = 12,
  TokenNotEqual     = 13,
  TokenGreater      = 14,
  TokenGreaterEqual = 15,
  TokenLess         = 16,
  TokenLessEqual    = 17,
  TokenIn           = 18,
  TokenPlus         = 19,
  TokenMinus        = 20,
  TokenStar         = 21,
  TokenSlash        = 22,
  TokenNot          = 23,
  TokenNum          = 24,
  TokenString       = 25,
  TokenTrue         = 26,
  TokenFalse        = 27,
  TokenNil          = 28,
  TokenColon        = 29,
  TokenDot          = 30,
  TokenDo           = 31,
  TokenEnd          = 32,
  TokenIf           = 33,
  TokenElse         = 34,
  TokenCond         = 35,
  TokenLBracket     = 36,
  TokenRBracket     = 37,
  TokenHashBracket  = 38,
  TokenLBrace       = 39,
  TokenRBrace       = 40,
  TokenNewline      = 41,
  TokenPipe         = 42,
  TokenModule       = 43,
} TokenType;

typedef struct {
  TokenType type;
  char *lexeme;
  u32 length;
  u32 line;
  u32 col;
} Token;

typedef struct {
  char *text;
  u32 pos;
  u32 line;
  u32 col;
  Token token;
} Lexer;

#define AtEnd(lex)  (PeekToken(lex).type == TokenEOF)

void InitLexer(Lexer *lex, char *text);
Token NextToken(Lexer *lex);
Token PeekToken(Lexer *lex);
void PrintToken(Token token);
// Val ParseNum(Token token);
