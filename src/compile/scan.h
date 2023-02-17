#pragma once
#include "../value.h"

typedef enum {
  TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACKET, TOKEN_RBRACKET, TOKEN_LBRACE,
  TOKEN_RBRACE, TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS, TOKEN_STAR,
  TOKEN_SLASH, TOKEN_BAR, TOKEN_EQ, TOKEN_NEQ, TOKEN_GT,
  TOKEN_GTE, TOKEN_LT, TOKEN_LTE, TOKEN_PIPE, TOKEN_ARROW, TOKEN_IDENTIFIER,
  TOKEN_STRING, TOKEN_NUMBER, TOKEN_AND, TOKEN_OR, TOKEN_DEF, TOKEN_IF,
  TOKEN_COND, TOKEN_DO, TOKEN_ELSE, TOKEN_END, TOKEN_TRUE, TOKEN_FALSE,
  TOKEN_NIL, TOKEN_NOT, TOKEN_NEWLINE, TOKEN_EOF, TOKEN_ERROR, TOKEN_COLON,
  TOKEN_MODULE, TOKEN_IMPORT, TOKEN_LET, TOKEN_USE,
} TokenType;

typedef struct {
  char *name;
  TokenType type;
} TokenMap;

typedef struct {
  TokenType type;
  u32 line;
  u32 col;
  char *lexeme;
  u32 length;
} Token;

typedef struct Parser Parser;

void AdvanceToken(Parser *p);
void PrintSourceContext(Parser *p, u32 num_lines);
char *TokenStr(TokenType type);
bool MatchToken(Parser *p, TokenType type);
void ExpectToken(Parser *p, TokenType type);
bool TokenEnd(Parser *p, TokenType type);
TokenType CurToken(Parser *p);
void PrintParser(Parser *p);
void SkipNewlines(Parser *p);
