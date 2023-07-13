#pragma once
#include "mem.h"
#include "source.h"

typedef enum {
  TokenEOF, TokenLet, TokenComma, TokenEqual, TokenDef, TokenLParen, TokenRParen, TokenID, TokenArrow, TokenAnd,
  TokenOr, TokenEqualEqual, TokenNotEqual, TokenGreater, TokenGreaterEqual, TokenLess, TokenLessEqual, TokenIn,
  TokenPlus, TokenMinus, TokenStar, TokenSlash, TokenNot, TokenNum, TokenString, TokenTrue, TokenFalse, TokenNil,
  TokenColon, TokenDot, TokenDo, TokenEnd, TokenIf, TokenElse, TokenCond, TokenLBracket, TokenRBracket,
  TokenHashBracket, TokenLBrace, TokenRBrace, TokenNewline, TokenPipe, TokenImport, TokenAs,
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
