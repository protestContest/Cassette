#pragma once
#include "value.h"

typedef enum {
  TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACKET, TOKEN_RBRACKET, TOKEN_LBRACE,
  TOKEN_RBRACE, TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS, TOKEN_STAR,
  TOKEN_SLASH, TOKEN_EXPONENT, TOKEN_BAR, TOKEN_EQ, TOKEN_NEQ, TOKEN_GT,
  TOKEN_GTE, TOKEN_LT, TOKEN_LTE, TOKEN_PIPE, TOKEN_ARROW, TOKEN_IDENTIFIER,
  TOKEN_STRING, TOKEN_NUMBER, TOKEN_SYMBOL, TOKEN_AND, TOKEN_OR, TOKEN_DEF,
  TOKEN_COND, TOKEN_DO, TOKEN_ELSE, TOKEN_END, TOKEN_NEWLINE, TOKEN_EOF,
  TOKEN_ERROR,
} TokenType;

typedef struct {
  char *name;
  TokenType type;
} TokenMap;

typedef struct {
  TokenType type;
  u32 line;
  u32 col;
  const char *start;
  u32 length;
} Token;

typedef struct {
  Status status;
  u32 line;
  u32 col;
  u32 cur;
  char *src;
  char *error;
} Reader;

Token ScanToken(Reader *r);

Reader *NewReader(char *src);
void FreeReader(Reader *r);

void Read(Reader *r, char *src);
void ReadFile(Reader *r, char *path);
void CancelRead(Reader *r);

void AppendSource(Reader *r, char *src);
Val Stop(Reader *r);
Val ParseError(Reader *r, char *msg);
void Rewind(Reader *r);
char Advance(Reader *r);
void AdvanceLine(Reader *r);
void Retreat(Reader *r, u32 count);

void PrintSource(Reader *r);
void PrintSourceContext(Reader *r, u32 num_lines);
void PrintReaderError(Reader *r);

bool IsSymChar(char c);
bool IsOperator(char c);
void SkipSpace(Reader *r);
void SkipSpaceAndNewlines(Reader *r);
bool Check(Reader *r, const char *expect);
bool CheckToken(Reader *r, const char *expect);
bool Match(Reader *r, const char *expect);
bool MatchToken(Reader *r, const char *expect);
void Expect(Reader *r, const char *expect);
