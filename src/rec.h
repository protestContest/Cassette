#pragma once

/********************************************\
*     ___    _____ ____  ____   ___    ____  *
*    / _ \  / ___// __/ / __ \ / _ \  / __ \ *
*   /   _/ / /__ / /   / / / //   _/ / / / / *
*  / /\ \ / /__ / /__ / /_/ // /\ \ / /_/ /  *
* /_/ /_//____//____/ \____//_/ /_//____,'   *
*                                            *
\********************************************/

#include "mem.h"

typedef enum {
  TokenEOF, TokenID, TokenBangEqual, TokenString, TokenNewline, TokenHash,
  TokenPercent, TokenLParen, TokenRParen, TokenStar, TokenPlus, TokenComma,
  TokenMinus, TokenArrow, TokenDot, TokenSlash, TokenNum, TokenColon, TokenLess,
  TokenLessEqual, TokenEqual, TokenEqualEqual, TokenGreater, TokenGreaterEqual,
  TokenLBracket, TokenRBracket, TokenAnd, TokenAs, TokenCond, TokenDef, TokenDo,
  TokenElse, TokenEnd, TokenFalse, TokenIf, TokenImport, TokenIn, TokenLet,
  TokenModule, TokenNil, TokenNot, TokenOr, TokenTrue, TokenLBrace, TokenBar,
  TokenRBrace
} TokenType;

typedef struct {
  TokenType type;
  char *lexeme;
  u32 length;
} Token;

typedef struct {
  char *source;
  u32 pos;
  Token token;
} Lexer;

void InitLexer(Lexer *lex, char *source);
Token NextToken(Lexer *lex);
