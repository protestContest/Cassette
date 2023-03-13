#pragma once
#include "../value.h"

typedef enum {
  TOKEN_EOF,
  TOKEN_ERROR,
  TOKEN_NUMBER,
  TOKEN_STRING,
  TOKEN_IDENT,
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
  TOKEN_OR,

  NUM_TOKENS
} TokenType;

typedef struct {
  u32 line;
  u32 col;
  u32 cur;
  char *data;
} Source;

typedef struct {
  TokenType type;
  u32 line;
  u32 col;
  char *lexeme;
  u32 length;
} Token;

void InitSource(Source *source, char *data);
Token ScanToken(Source *source);
char *TokenStr(TokenType type);
void PrintSourceContext(Source *source, Token cur_token, u32 num_lines);
