#include "error.h"
#include "univ/file.h"
#include "univ/math.h"
#include "univ/str.h"

Error **NewError(char *message, char *filename, u32 pos, u32 length)
{
  Error **error = New(Error);
  (*error)->message = NewString(message);
  (*error)->filename = filename ? NewString(filename) : 0;
  (*error)->pos = pos;
  (*error)->length = length;
  (*error)->data = 0;
  return error;
}

void FreeError(Error **error)
{
  if ((*error)->message) DisposeHandle((Handle)(*error)->message);
  if ((*error)->filename) DisposeHandle((Handle)(*error)->filename);
  DisposeHandle((Handle)error);
}

static void PrintSourceContext(char *text, u32 pos, u32 length, u32 context)
{
  char *line = LineStart(pos, text);
  char *startline = line;
  char *endline = line;
  char *lineend;
  u32 col = ColNum(text, pos);
  u32 i, linenum_width, len, linenum;
  for (i = 0; i < context && startline > text; i++) {
    startline = LineStart(startline-text-1, text);
    endline = LineEnd(endline-text, text);
  }
  linenum_width = NumDigits(LineNum(text, endline-text), 10);

  while (startline < line) {
    linenum = LineNum(text, startline-text);
    lineend = LineEnd(startline-text, text);
    len = lineend - startline;
    fprintf(stderr, " %*d│ %*.*s", linenum_width, linenum+1, len, len, startline);
    startline = lineend;
  }

  linenum = LineNum(text, line-text);
  lineend = LineEnd(line-text, text);
  len = lineend - line;
  fprintf(stderr, " %*d│ %*.*s", linenum_width, linenum+1, len, len, line);
  for (i = 0; i < linenum_width + 3 + col; i++) fprintf(stderr, " ");
  if (length == 0) length = 1;
  for (i = 0; i < length; i++) fprintf(stderr, "^");
  fprintf(stderr, "\n");

  line = LineEnd(line-text, text);
  while (line < endline) {
    linenum = LineNum(text, line-text);
    lineend = LineEnd(line-text, text);
    len = lineend - line;
    fprintf(stderr, " %*d│ %*.*s", linenum_width, linenum+1, len, len, line);
    line = lineend;
  }
  linenum = LineNum(text, line-text);
  lineend = LineEnd(line-text, text);
  len = lineend - line;
  fprintf(stderr, " %*d│ %*.*s", linenum_width, linenum+1, len, len, line);
}

void PrintSourceLine(char *text, u32 pos)
{
  char *line = LineStart(pos, text);
  char *end = LineEnd(pos, text);
  u32 len = end - line;
  fprintf(stderr, "%*.*s", len, len, line);
}

void PrintError(Error **error)
{
  char **text = 0;
  u32 line, col;
  if ((*error)->filename) {
    text = ReadFile(*(*error)->filename);
    if (text) {
      line = LineNum(*text, (*error)->pos);
      col = ColNum(*text, (*error)->pos);
      fprintf(stderr, "%s:%d:%d: ", *(*error)->filename, line+1, col+1);
    }
  }
  fprintf(stderr, "%s\n", *(*error)->message);

  if (text) PrintSourceContext(*text, (*error)->pos, (*error)->length, 1);

  fprintf(stderr, "\n");
  if(text) DisposeHandle((Handle)text);
}
