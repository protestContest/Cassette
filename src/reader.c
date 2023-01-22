#include "reader.h"
#include "parse.h"
#include "mem.h"
#include "printer.h"
#include <stdio.h>

Reader *NewReader(void)
{
  Reader *r = malloc(sizeof(Reader));
  r->status = PARSE_OK;
  r->file = "stdin";
  r->line = 1;
  r->col = 1;
  r->src = NULL;
  r->cur = 0;
  r->indent = 0;
  r->error = NULL;
  r->ast = nil;
  return r;
}

void FreeReader(Reader *r)
{
  free(r->src);
  free(r->error);
  free(r);
}

void Read(Reader *r, char *src)
{
  AppendSource(r, src);

  Val exp = Parse(r);
  if (r->status != PARSE_OK) return;

  r->ast = exp;

  if (DEBUG_PARSE) {
    fprintf(stderr, "AST: ");
    PrintVal(r->ast);
  }
}

void ReadFile(Reader *reader, char *path)
{
  FILE *file = fopen(path, "r");
  if (!file) {
    u32 len = snprintf(NULL, 0, "Could not open file \"%s\"", path);
    char msg[len+1];
    sprintf(msg, "Could not open file \"%s\"", path);
    ParseError(reader, msg);
    return;
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

  reader->file = path;
  Read(reader, src);
}

void CancelRead(Reader *r)
{
  r->status = PARSE_OK;
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
  r->status = PARSE_INCOMPLETE;
  return nil;
}

Val ParseError(Reader *r, char *msg)
{
  r->status = PARSE_ERROR;
  r->error = realloc(r->error, strlen(msg) + 1);
  strcpy(r->error, msg);
  return nil;
}

void Rewind(Reader *r)
{
  r->cur = 0;
  r->col = 1;
  r->line = 1;
  r->status = PARSE_OK;
}

char Advance(Reader *r)
{
  r->col++;
  return r->src[r->cur++];
}

void AdvanceLine(Reader *r)
{
  r->line++;
  r->col = 1;
  r->cur++;
}

void Retreat(Reader *r, u32 count)
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
  if (r->status != PARSE_ERROR) return;

  // red text
  fprintf(stderr, "\x1B[31m");
  fprintf(stderr, "[%s:%d:%d] Parse error: %s\n\n", r->file, r->line, r->col, r->error);

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

bool CheckToken(Reader *r, const char *expect)
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

void Expect(Reader *r, const char *expect)
{
  if (!Match(r, expect)) {
    char *msg = NULL;
    PrintInto(msg, "Expected \"%s\"", expect);
    ParseError(r, msg);
  }
}
