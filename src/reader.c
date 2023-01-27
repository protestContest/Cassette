#include "reader.h"
#include "mem.h"
#include "printer.h"
#include <stdio.h>

#define IsSpace(c)        ((c) == ' ' || (c) == '\t')
#define IsNewline(c)      ((c) == '\n' || (c) == '\r')
#define IsAlpha(c)        (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' & (c) <= 'z'))
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsEnd(c)          ((c) == '\0')
#define IsBoundary(c)     (IsSpace(c) || IsNewline(c) || IsEnd(c))

#define Peek(r)           ((r)->src[(r)->cur])
#define PeekNext(r)   ((r)->src[(r)->cur + 1])

static char Advance(Reader *r);
static void AdvanceLine(Reader *r);
static void Retreat(Reader *r, u32 count);

Token NumberToken(Reader *r);
Token StringToken(Reader *r);
Token IdentifierToken(Reader *r);

static Token MakeToken(Reader *r, TokenType type, u32 length)
{
  Token token;
  token.type = type;
  token.line = r->line;
  token.col = r->col;
  token.lexeme = r->src + r->cur - length;
  token.length = length;
  return token;
}

static Token ErrorToken(Reader *r, const char *msg)
{
  Token token;
  token.type = TOKEN_ERROR;
  token.line = r->line;
  token.col = r->col;
  token.lexeme = msg;
  token.length = strlen(msg);
  return token;
}

Reader *NewReader(char *src)
{
  Reader *r = malloc(sizeof(Reader));
  InitReader(r, src);
  return r;
}

Reader *InitReader(Reader *r, char *src)
{
  r->status = Ok;
  r->line = 1;
  r->col = 1;
  r->src = src;
  r->cur = 0;
  r->error = NULL;
  return r;
}

void FreeReader(Reader *r)
{
  free(r->src);
  free(r->error);
  free(r);
}

Token ScanToken(Reader *r)
{
  SkipSpace(r);

  if (IsEnd(Peek(r))) return MakeToken(r, TOKEN_EOF, 0);

  if (IsDigit(Peek(r))) return NumberToken(r);
  if (Match(r, "\""))   return StringToken(r);

  if (Match(r, "("))  return MakeToken(r, TOKEN_LPAREN, 1);
  if (Match(r, ")"))  return MakeToken(r, TOKEN_RPAREN, 1);
  if (Match(r, "["))  return MakeToken(r, TOKEN_LBRACKET, 1);
  if (Match(r, "]"))  return MakeToken(r, TOKEN_RBRACKET, 1);
  if (Match(r, "{"))  return MakeToken(r, TOKEN_LBRACE, 1);
  if (Match(r, "}"))  return MakeToken(r, TOKEN_RBRACE, 1);
  if (Match(r, ","))  return MakeToken(r, TOKEN_COMMA, 1);
  if (Match(r, "."))  return MakeToken(r, TOKEN_DOT, 1);
  if (Match(r, "-"))  return MakeToken(r, TOKEN_MINUS, 1);
  if (Match(r, "+"))  return MakeToken(r, TOKEN_PLUS, 1);
  if (Match(r, "*"))  return MakeToken(r, TOKEN_STAR, 1);
  if (Match(r, "/"))  return MakeToken(r, TOKEN_SLASH, 1);
  if (Match(r, "|"))  return MakeToken(r, TOKEN_BAR, 1);
  if (Match(r, "="))  return MakeToken(r, TOKEN_EQ, 1);
  if (Match(r, ">"))  return MakeToken(r, TOKEN_GT, 1);
  if (Match(r, "<"))  return MakeToken(r, TOKEN_LT, 1);
  if (Match(r, "\n")) return MakeToken(r, TOKEN_NEWLINE, 1);
  if (Match(r, "\r")) return MakeToken(r, TOKEN_NEWLINE, 1);

  if (Match(r, "!=")) return MakeToken(r, TOKEN_NEQ, 2);
  if (Match(r, ">=")) return MakeToken(r, TOKEN_GTE, 2);
  if (Match(r, "**")) return MakeToken(r, TOKEN_EXPONENT, 2);
  if (Match(r, "|>")) return MakeToken(r, TOKEN_PIPE, 2);
  if (Match(r, "->")) return MakeToken(r, TOKEN_ARROW, 2);
  if (Match(r, "<=")) return MakeToken(r, TOKEN_LTE, 2);

  if (IsSymChar(Peek(r))) return IdentifierToken(r);

  return ErrorToken(r, "Unexpected character");
}

Token NumberToken(Reader *r)
{
  u32 start = r->cur;

  while (IsDigit(Peek(r)) || Peek(r) == '_') Advance(r);
  if (Check(r, ".") && IsDigit(PeekNext(r))) {
    Match(r, ".");
    while (IsDigit(Peek(r)) || Peek(r) == '_') Advance(r);
  }

  return MakeToken(r, TOKEN_NUMBER, r->cur - start);
}

Token StringToken(Reader *r)
{
  u32 start = r->cur - 1;
  while (Peek(r) != '"' && !IsEnd(Peek(r))) {
    if (Peek(r) == '\n') AdvanceLine(r);
    else Advance(r);
  }

  if (IsEnd(Peek(r))) return ErrorToken(r, "Unterminated string");
  Advance(r);
  return MakeToken(r, TOKEN_STRING, r->cur - start);
}

Token IdentifierToken(Reader *r)
{
  u32 start = r->cur;

  if (MatchKeyword(r, "and"))   return MakeToken(r, TOKEN_AND, 3);
  if (MatchKeyword(r, "cond"))  return MakeToken(r, TOKEN_COND, 4);
  if (MatchKeyword(r, "def"))   return MakeToken(r, TOKEN_DEF, 3);
  if (MatchKeyword(r, "do"))    return MakeToken(r, TOKEN_DO, 2);
  if (MatchKeyword(r, "else"))  return MakeToken(r, TOKEN_ELSE, 4);
  if (MatchKeyword(r, "end"))   return MakeToken(r, TOKEN_END, 3);
  if (MatchKeyword(r, "or"))    return MakeToken(r, TOKEN_OR, 2);

  while (IsSymChar(Peek(r))) Advance(r);

  if (r->cur == start) return ErrorToken(r, "Expected symbol");

  return MakeToken(r, TOKEN_IDENTIFIER, r->cur - start);
}



Val ParseIdentifier(Reader *r)
{
  // DebugParse(r, "ParseIdentifier");

  u32 start = r->cur;
  while (IsSymChar(Peek(r))) {
    Advance(r);
  }

  if (r->cur == start) {
    ParseError(r, "Expected symbol");
    return nil;
  }

  Val exp = MakeSymbolFromSlice(&r->src[start], r->cur - start);
  // DebugResult(r, exp);
  return exp;
}

void AppendSource(Reader *r, char *src)
{
  if (r->src == NULL) {
    r->src = malloc(strlen(src) + 1);
    strcpy(r->src, src);
  } else {
    r->src = realloc(r->src, strlen(r->src) + strlen(src) + 1);
    strcat(r->src, src);
  }
}

Val Stop(Reader *r)
{
  r->status = Unknown;
  return nil;
}

Val ParseError(Reader *r, char *msg)
{
  r->status = Error;
  r->error = realloc(r->error, strlen(msg) + 1);
  strcpy(r->error, msg);
  return nil;
}

void Rewind(Reader *r)
{
  r->cur = 0;
  r->col = 1;
  r->line = 1;
  r->status = Ok;
}

static char Advance(Reader *r)
{
  r->col++;
  return r->src[r->cur++];
}

static void AdvanceLine(Reader *r)
{
  r->line++;
  r->col = 1;
  r->cur++;
}

static void Retreat(Reader *r, u32 count)
{
  while (count > 0 && r->cur > 0) {
    r->cur--;
    r->col--;
    count--;
  }
}

void PrintSource(Reader *r)
{
  u32 linenum_size = (r->line >= 1000) ? 4 : (r->line >= 100) ? 3 : (r->line >= 10) ? 2 : 1;
  u32 colnum_size = (r->col >= 1000) ? 4 : (r->col >= 100) ? 3 : (r->col >= 10) ? 2 : 1;
  u32 gutter = (linenum_size > colnum_size + 1) ? linenum_size : colnum_size + 1;
  if (gutter < 3) gutter = 3;

  char *c = r->src;
  u32 line = 1;

  while (!IsEnd(*c)) {
    fprintf(stderr, "  %*d │ ", gutter, line);
    while (!IsNewline(*c) && !IsEnd(*c)) {
      fprintf(stderr, "%c", *c);
      c++;
    }

    fprintf(stderr, "\n");
    line++;
    c++;
  }

  if (IsEnd(*c) && c > r->src && !IsNewline(*(c-1))) {
    fprintf(stderr, "\n");
  }
}

void PrintSourceContext(Reader *r, u32 num_lines)
{
  char *c = &r->src[r->cur];
  u32 after = 0;
  while (!IsEnd(*c) && after < num_lines / 2) {
    if (IsNewline(*c)) after++;
    c++;
  }

  u32 before = num_lines - after;
  if (before > r->line) {
    before = r->line;
    after = num_lines - r->line;
  }

  u32 gutter = (r->line >= 1000) ? 4 : (r->line >= 100) ? 3 : (r->line >= 10) ? 2 : 1;
  if (gutter < 2) gutter = 2;

  c = r->src;
  i32 line = 1;

  while (line < (i32)r->line - (i32)before) {
    if (IsEnd(*c)) return;
    if (IsNewline(*c)) line++;
    c++;
  }

  while (line < (i32)r->line + (i32)after + 1 && !IsEnd(*c)) {
    fprintf(stderr, "  %*d │ ", gutter, line);
    while (!IsNewline(*c) && !IsEnd(*c)) {
      fprintf(stderr, "%c", *c);
      c++;
    }

    if (line == (i32)r->line) {
      fprintf(stderr, "\n  ");
      for (u32 i = 0; i < gutter + r->col + 2; i++) fprintf(stderr, " ");
      fprintf(stderr, "↑");
    }
    fprintf(stderr, "\n");
    line++;
    c++;
  }

  if (IsEnd(*c) && c > r->src && !IsNewline(*(c-1))) {
    fprintf(stderr, "\n");
  }
}

void PrintReaderError(Reader *r)
{
  if (r->status != Error) return;

  // red text
  fprintf(stderr, "\x1B[31m");
  fprintf(stderr, "[%d:%d] Parse error: %s\n\n", r->line, r->col, r->error);

  PrintSourceContext(r, 10);

  // reset text
  fprintf(stderr, "\x1B[0m");
}

bool IsSymChar(char c)
{
  if (IsAlpha(c)) return true;
  if (IsSpace(c)) return false;
  if (IsNewline(c)) return false;
  if (IsEnd(c)) return false;

  switch (c) {
  case '(':
  case ')':
  case '{':
  case '}':
  case '[':
  case ']':
  case '"':
  case ':':
  case ';':
  case '.':
  case ',':
    return false;
  default:
    return true;
  }
}

bool IsOperator(char c)
{
  switch (c) {
  case '+':
  case '-':
  case '*':
  case '/':
    return true;
  default:
    return false;
  }
}

void SkipSpace(Reader *r)
{
  while (IsSpace(Peek(r))) {
    Advance(r);
  }

  if (Peek(r) == ';') {
    while (!IsNewline(Peek(r))) {
      Advance(r);
    }
    AdvanceLine(r);
    SkipSpace(r);
  }
}

void SkipSpaceAndNewlines(Reader *r)
{
  SkipSpace(r);
  while (IsNewline(Peek(r))) {
    AdvanceLine(r);
    SkipSpace(r);
  }
}

bool Check(Reader *r, const char *expect)
{
  u32 len = strlen(expect);

  for (u32 i = 0; i < len; i++) {
    if IsEnd(r->src[r->cur + i]) return false;
    if (r->src[r->cur + i] != expect[i]) return false;
  }

  return true;
}

bool CheckKeyword(Reader *r, const char *expect)
{
  if (!Check(r, expect)) return false;
  if (IsSymChar(r->src[r->cur + strlen(expect)])) return false;
  return true;
}

bool Match(Reader *r, const char *expect)
{
  if (Check(r, expect)) {
    r->cur += strlen(expect);
    r->col += strlen(expect);
    SkipSpace(r);
    return true;
  } else {
    return false;
  }
}

bool MatchKeyword(Reader *r, const char *expect)
{
  if (CheckKeyword(r, expect)) {
    Match(r, expect);
    return true;
  } else {
    return false;
  }
}
