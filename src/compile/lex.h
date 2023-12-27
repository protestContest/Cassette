#pragma once

typedef enum {
  TokenEOF, TokenNewline, TokenBangEq, TokenStr, TokenHash, TokenPercent,
  TokenAmpersand, TokenLParen, TokenRParen, TokenStar, TokenPlus, TokenComma,
  TokenMinus, TokenArrow, TokenDot, TokenSlash, TokenNum, TokenColon, TokenLt,
  TokenLtLt, TokenLtEq, TokenLtGt, TokenEq, TokenEqEq, TokenGt, TokenGtEq,
  TokenGtGt, TokenLBracket, TokenBackslash, TokenRBracket, TokenCaret, TokenAnd,
  TokenAs, TokenCond, TokenDef, TokenDo, TokenElse, TokenEnd,
  TokenFalse, TokenIf, TokenImport, TokenIn, TokenLet, TokenModule,
  TokenNil, TokenNot, TokenOr, TokenTrue, TokenLBrace, TokenBar,
  TokenRBrace, TokenTilde, TokenID
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

#define LexPeek(lex)  ((lex)->token.type)
#define TokenPos(lex) ((lex)->token.lexeme - (lex)->source)

void InitLexer(Lexer *lex, char *source, u32 pos);
Token NextToken(Lexer *lex);
bool MatchToken(TokenType type, Lexer *lex);
void SkipNewlines(Lexer *lex);
