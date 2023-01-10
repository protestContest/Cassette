#include "reader.h"
#include "mem.h"

typedef struct {
  char *src;
  u32 cur;
  u32 line;
  u32 col;
} Reader;

#define IsSpace(c)        ((c) == ' ' || (c) == '\n' || (c) == '\r' || (c) == '\t')
#define IsAlpha(c)        (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' & (c) <= 'z'))
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsUTFChar(c)      (((c) & 0x80) == 0x80)
#define IsEnd(c)          ((c) == '\0')

static void SkipSpace(Reader *r);
static bool Match(Reader *r, char *expected);
static bool IsSymChar(char c);

static Val ParseExpr(Reader *r);
static Val ParseInt(Reader *r);
static Val ParseString(Reader *r);
static Val ParseSymbol(Reader *r);
static Val ParseVariable(Reader *r);
static Val ParseBlock(Reader *r);
static Val ParseList(Reader *r);
static Val ParseListLiteral(Reader *r);
static Val ParseDict(Reader *r);

static void ParseError(Reader *r, char *message);

Val Read(char *src)
{
  Reader r = { src, 0, 1, 1};
  return ParseExpr(&r);
}

Val ReadFile(char *path)
{
  FILE *file = fopen(path, "r");
  if (!file) {
    Error("Could not open file \"%s\"", path);
  }

  fseek(file, 0, SEEK_END);
  u32 size = ftell(file);
  rewind(file);

  char src[size+1];
  for (u32 i = 0; i < size; i++) {
    int c = fgetc(file);
    src[i] = (char)c;
  }
  src[size] = '\0';

  return Read(src);
}

static void Advance(Reader *r)
{
  r->col++;
  r->cur++;
}

static void AdvanceLine(Reader *r)
{
  r->line++;
  r->col = 1;
  r->cur++;
}

static void SkipSpace(Reader *r)
{
  while (IsSpace(r->src[r->cur])) {
    if (r->src[r->cur] == '\n') {
      AdvanceLine(r);
    } else {
      Advance(r);
    }
  }

  if (r->src[r->cur] == ';') {
    while (r->src[r->cur] != '\n') {
      Advance(r);
    }
    SkipSpace(r);
  }
}

static bool Match(Reader *r, char *expect)
{
  SkipSpace(r);
  u32 len = strlen(expect);
  for (u32 i = 0; i < len; i++) {
    if IsEnd(r->src[r->cur + i]) return false;
    if (r->src[r->cur + i] != expect[i]) return false;
  }
  Advance(r);
  SkipSpace(r);
  return true;
}

static bool IsSymChar(char c)
{
  if (IsAlpha(c)) return true;
  if (IsSpace(c)) return false;

  switch (c) {
  case '(':
  case ')':
  case '{':
  case '}':
  case ':':
  case ';':
    return false;
  default:
    return true;
  }
}

static Val ParseExpr(Reader *r)
{
  SkipSpace(r);

  if (IsEnd(r->src[r->cur])) {
    Error("Unexpected end");
  }

  if (IsDigit(r->src[r->cur])) return ParseInt(r);
  if (Match(r, "«")) return ParseString(r);
  if (Match(r, "do")) return ParseBlock(r);
  if (Match(r, ":")) return ParseSymbol(r);

  if (Match(r, "(")) {
    Val exp = ParseList(r);

    if (Match(r, "->")) {
      Val body = ParseExpr(r);
      return MakeList(3, MakeSymbol("λ", 2), exp, body);
    } else {
      return exp;
    }
  }

  if (Match(r, "[")) {
    return ParseListLiteral(r);
  }

  if (Match(r, "{")) {
    return ParseDict(r);
  }

  if (IsSymChar(r->src[r->cur])) {
    return ParseVariable(r);
  }

  ParseError(r, "Unexpected character");
  return nil;
}

static Val ParseInt(Reader *r)
{
  u32 n = r->src[r->cur] - '0';
  Advance(r);
  while (IsDigit(r->src[r->cur])) {
    u32 d = r->src[r->cur] - '0';
    n = n*10 + d;
    Advance(r);
    while (r->src[r->cur] == '_') Advance(r);
  }
  return IntVal(n);
}

static void SkipString(Reader *r)
{
  while (!Match(r, "»")) {
    if (IsEnd(r->src[r->cur])) Error("Unterminated string");
    if (Match(r, "«")) SkipString(r);
    if (r->src[r->cur] == '\n') {
      AdvanceLine(r);
    } else {
      Advance(r);
    }
  }
}

static Val ParseString(Reader *r)
{
  u32 start = r->cur;

  SkipString(r);

  u32 length = r->cur - start;
  return MakeBinary(r->src + start, length);
}

static Val ParseVariable(Reader *r)
{
  u32 start = r->cur;
  while (IsSymChar(r->src[r->cur])) {
    Advance(r);
  }

  if (r->cur == start) {
    ParseError(r, "Expected symbol");
  }

  Val symbol = MakeSymbol(&r->src[start], r->cur - start);

  return symbol;
}

static Val ParseSymbol(Reader *r)
{
  Val symbol = ParseVariable(r);
  return MakePair(MakeSymbol("quote", 5), symbol);
}

static Val ParseBlock(Reader *r)
{
  Val block = MakePair(MakeSymbol("do", 2), nil);

  while (!Eq(Head(block), SymbolFor("end"))) {
    block = MakePair(ParseExpr(r), block);
  }

  return Reverse(Tail(block));
}

static Val ParseList(Reader *r)
{
  Val items = nil;

  while (!Match(r, ")")) {
    items = MakePair(ParseExpr(r), items);
  }

  return Reverse(items);
}

static Val ParseListLiteral(Reader *r)
{
  Val items = nil;

  while (!Match(r, "]")) {
    items = MakePair(ParseExpr(r), items);
  }

  return MakeList(2, MakeSymbol("list", 5), Reverse(items));
}

static Val ParseDict(Reader *r)
{
  Val keys = nil;
  Val vals = nil;

  while (!Match(r, "}")) {
    keys = MakePair(ParseSymbol(r), keys);

    if (!Match(r, ":")) {
      ParseError(r, "Expected ':'");
    }

    vals = MakePair(ParseExpr(r), vals);
  }

  return MakeList(3, MakeSymbol("dict", 4), keys, vals);
}

static void ParseError(Reader *r, char *message)
{
  printf("[%d:%d] Error: %s\n\n", r->line, r->col, message);

  char *line = r->src + r->cur - (r->col - 1);

  printf("  ");
  while (*line != '\n' && *line != '\0') {
    printf("%c", *line++);
  }

  printf("\n");
  printf("  ");
  for (u32 i = 0; i < r->col - 1; i++) printf(" ");
  printf("^\n");

  exit(1);
}
