#pragma once

typedef enum {
  TokenEOF, TokenID, TokenBangEqual, TokenString, TokenNewline, TokenHash,
  TokenPercent, TokenLParen, TokenRParen, TokenStar, TokenPlus, TokenComma,
  TokenMinus, TokenArrow, TokenDot, TokenSlash, TokenNum, TokenColon, TokenLess,
  TokenLessEqual, TokenEqual, TokenEqualEqual, TokenGreater, TokenGreaterEqual,
  TokenLBracket, TokenRBracket, TokenAnd, TokenAs, TokenCond, TokenDef, TokenDo,
  TokenElse, TokenEnd, TokenFalse, TokenIf, TokenImport, TokenIn, TokenLet,
  TokenModule, TokenNil, TokenNot, TokenOr, TokenTrue, TokenLBrace, TokenBar,
  TokenRBrace, NumTokenTypes
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

void InitLexer(Lexer *lex, char *source, u32 pos);
Token NextToken(Lexer *lex);
bool MatchToken(TokenType type, Lexer *lex);
void SkipNewlines(Lexer *lex);
void PrintToken(Token token);
