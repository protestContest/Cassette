#pragma once

typedef enum {
  TOKEN_EOF,
  TOKEN_ERROR,
  TOKEN_NUMBER,
  TOKEN_STRING,
  TOKEN_ID,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LBRACKET,
  TOKEN_RBRACKET,
  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_DOT,
  TOKEN_STAR,
  TOKEN_SLASH,
  TOKEN_MINUS,
  TOKEN_PLUS,
  TOKEN_BAR,
  TOKEN_GREATER,
  TOKEN_LESS,
  TOKEN_EQUAL,
  TOKEN_COMMA,
  TOKEN_COLON,
  TOKEN_NEWLINE,
  TOKEN_ARROW,
  TOKEN_AND,
  TOKEN_COND,
  TOKEN_DEF,
  TOKEN_DO,
  TOKEN_ELSE,
  TOKEN_END,
  TOKEN_IF,
  TOKEN_NOT,
  TOKEN_OR
} TokenType;

typedef struct {
  TokenType type;
  u32 line;
  u32 col;
  char *lexeme;
  u32 length;
} Token;

typedef struct {
  char *src;
  u32 pos;
  u32 line;
  u32 col;
} Lexer;

void InitLexer(Lexer *lexer, char *src);
Token NextToken(Lexer *lexer);
void PrintToken(Token token);
