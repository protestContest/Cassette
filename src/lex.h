#pragma once
#include "value.h"

typedef enum {
  ParseSymEOF = 0,
  ParseSymPlus = 1,
  ParseSymMinus = 2,
  ParseSymStar = 3,
  ParseSymSlash = 4,
  ParseSymNum = 5,
  ParseSymID = 6,
  ParseSymLParen = 7,
  ParseSymRParen = 8,
  ParseSymArrow = 9,
  ParseSymProgram = 10,
  ParseSymExpr = 11,
  ParseSymCall = 12,
  ParseSymArg = 13,
  ParseSymSum = 14,
  ParseSymProduct = 15,
  ParseSymPrimary = 16,
  ParseSymGroup = 17,
  ParseSymLambda = 18,
} ParseSymbol;

typedef struct {
  u32 type;
  u32 line;
  u32 col;
  char *lexeme;
  u32 length;
  Val value;
} Token;

struct Lexer;
typedef Token (*NextTokenFn)(struct Lexer *lexer);

typedef struct Lexer {
  char *src;
  u32 start;
  u32 pos;
  u32 line;
  u32 col;
  NextTokenFn next_token;
} Lexer;

#define LexPeek(lexer, n) (lexer)->src[(lexer)->pos + n]
#define IsWhitespace(c)   ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')
#define IsNewline(c)      ((c) == '\r' || (c) == '\n')
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsHexDigit(c)     (IsDigit(c) || ((c) >= 'A' && (c) <= 'F'))
#define IsUppercase(c)    ((c) >= 'A' && (c) <= 'Z')
#define IsLowercase(c)    ((c) >= 'a' && (c) <= 'z')
#define IsAlpha(c)        (IsUpper(c) || IsLower(c))

#define IsErrorToken(t)   ((t).type == (u32)-1)

void InitLexer(Lexer *lexer, NextTokenFn next_token, char *src);
Token NextToken(Lexer *lexer);
Token RyeToken(Lexer *lexer);
int PrintToken(Token token);
int DebugToken(Token token);
void PrintSourceContext(Lexer *lexer, u32 num_lines);

Token MakeToken(u32 type, Lexer *lexer, Val value);
Token ErrorToken(Lexer *lexer, char *msg);
bool IsIDChar(char c);
void SkipWhitespace(Lexer *lexer);
bool Match(Lexer *lexer, char *expected);
bool MatchKeyword(Lexer *lexer, char *expected);
