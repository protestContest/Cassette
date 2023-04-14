#pragma once
#include "value.h"
#include "mem.h"

enum {
  ParseSymEOF,
  ParseSymDef,
  ParseSymID,
  ParseSymIf,
  ParseSymDo,
  ParseSymElse,
  ParseSymEnd,
  ParseSymPlus,
  ParseSymMinus,
  ParseSymStar,
  ParseSymSlash,
  ParseSymNUM,
  ParseSymLParen,
  ParseSymRParen,
  ParseSymArrow,
  ParseSymNL,
  ParseSymProgram,
  ParseSymBlock,
  ParseSymStmt,
  ParseSymExpr,
  ParseSymIf_block,
  ParseSymCall,
  ParseSymSum,
  ParseSymProduct,
  ParseSymPrimary,
  ParseSymDo_block,
  ParseSymGroup,
  ParseSymLambda,
  ParseSymNewlines,
};

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
  Mem *mem;
  NextTokenFn next_token;
} Lexer;

#define LexPeek(lexer, n) (lexer)->src[(lexer)->pos + n]
#define IsWhitespace(c)   ((c) == ' ' || (c) == '\t')
#define IsNewline(c)      ((c) == '\r' || (c) == '\n')
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsHexDigit(c)     (IsDigit(c) || ((c) >= 'A' && (c) <= 'F'))
#define IsUppercase(c)    ((c) >= 'A' && (c) <= 'Z')
#define IsLowercase(c)    ((c) >= 'a' && (c) <= 'z')
#define IsAlpha(c)        (IsUpper(c) || IsLower(c))

#define IsErrorToken(t)   ((t).type == (u32)-1)

void InitLexer(Lexer *lexer, NextTokenFn next_token, char *src, Mem *mem);
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
