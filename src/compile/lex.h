#pragma once

typedef enum {
  TOKEN_EOF,
  TOKEN_NUMBER,
  TOKEN_ID,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_STAR,
  TOKEN_SLASH,
  TOKEN_MINUS,
  TOKEN_PLUS,
  TOKEN_ARROW,
} TokenType;

// typedef enum {
//   TOKEN_EOF,
//   TOKEN_ERROR,
//   TOKEN_NUMBER,
//   TOKEN_STRING,
//   TOKEN_ID,
//   TOKEN_LPAREN,
//   TOKEN_RPAREN,
//   TOKEN_LBRACKET,
//   TOKEN_RBRACKET,
//   TOKEN_LBRACE,
//   TOKEN_RBRACE,
//   TOKEN_DOT,
//   TOKEN_STAR,
//   TOKEN_SLASH,
//   TOKEN_MINUS,
//   TOKEN_PLUS,
//   TOKEN_BAR,
//   TOKEN_GREATER,
//   TOKEN_LESS,
//   TOKEN_EQUAL,
//   TOKEN_COMMA,
//   TOKEN_COLON,
//   TOKEN_COLON_COLON,
//   TOKEN_NEWLINE,
//   TOKEN_ARROW,
//   TOKEN_AND,
//   TOKEN_COND,
//   TOKEN_DEF,
//   TOKEN_DO,
//   TOKEN_ELSE,
//   TOKEN_END,
//   TOKEN_IF,
//   TOKEN_NOT,
//   TOKEN_OR
// } TokenType;

#define NUM_TOKENS 10

typedef struct {
  u32 type;
  u32 line;
  u32 col;
  char *lexeme;
  u32 length;
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
#define IsAlpha(c)        (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))

#define IsErrorToken(t)   ((t).type == (u32)-1)

void InitLexer(Lexer *lexer, NextTokenFn next_token, char *src);
Token NextToken(Lexer *lexer);
Token RyeToken(Lexer *lexer);
int PrintToken(Token token);
int DebugToken(Token token);
void PrintSourceContext(Lexer *lexer, u32 num_lines);

Token MakeToken(u32 type, Lexer *lexer);
Token ErrorToken(Lexer *lexer, char *msg);
bool IsIDChar(char c);
void SkipWhitespace(Lexer *lexer);
bool Match(Lexer *lexer, char *expected);
bool MatchKeyword(Lexer *lexer, char *expected);
