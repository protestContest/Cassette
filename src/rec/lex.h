#pragma once

typedef enum {
  TokenEOF,
  TokenID,
  TokenBangEqual,
  TokenString,
  TokenNewline,
  TokenHash,
  TokenPercent,
  TokenLParen,
  TokenRParen,
  TokenStar,
  TokenPlus,
  TokenComma,
  TokenMinus,
  TokenArrow,
  TokenDot,
  TokenSlash,
  TokenNum,
  TokenColon,
  TokenLess,
  TokenLessEqual,
  TokenEqual,
  TokenEqualEqual,
  TokenGreater,
  TokenGreaterEqual,
  TokenLBracket,
  TokenRBracket,
  TokenAnd,
  TokenAs,
  TokenCond,
  TokenDef,
  TokenDo,
  TokenElse,
  TokenEnd,
  TokenFalse,
  TokenIf,
  TokenImport,
  TokenIn,
  TokenLet,
  TokenModule,
  TokenNil,
  TokenNot,
  TokenOr,
  TokenTrue,
  TokenLBrace,
  TokenBar,
  TokenRBrace,
} TokenType;

typedef struct {
  TokenType type;
  char *lexeme;
  u32 length;
} Token;

typedef struct {
  char *text;
  u32 pos;
  Token token;
} Lexer;

#define AtEnd(lex)  (PeekToken(lex).type == TokenEOF)
void InitLexer(Lexer *lex, char *text);
Token NextToken(Lexer *lex);
Token PeekToken(Lexer *lex);
