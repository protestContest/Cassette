#include "result.h"
#include "univ/math.h"
#include <univ/str.h>
#include <univ/file.h>

Result Ok(void *data)
{
  Result result;
  result.ok = true;
  result.data.p = data;
  return result;
}

Result OkVal(u32 value)
{
  Result result;
  result.ok = true;
  result.data.v = value;
  return result;
}

Result Err(Error *error)
{
  Result result;
  result.ok = false;
  result.data.p = error;
  return result;
}

Error *NewError(char *message, char *file, u32 pos, u32 length)
{
  Error *error = malloc(sizeof(Error));
  error->message = message ? strndup(message, strlen(message)) : 0;
  error->file = file;
  error->pos = pos;
  error->length = length;
  return error;
}

void FreeError(Error *error)
{
  free(error->message);
  free(error);
}

void PrintSourceContext(char *text, u32 pos, u32 length, u32 context)
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

void PrintError(Result result)
{
  Error *error = result.data.p;
  char *text = 0;
  u32 line, col;
  fprintf(stderr, "%s", ANSIRed);
  if (error->file) {
    text = ReadFile(error->file);
    if (text) {
      line = LineNum(text, error->pos);
      col = ColNum(text, error->pos);
      fprintf(stderr, "%s:%d:%d: ", error->file, line+1, col+1);
    }
  }
  fprintf(stderr, "%s\n", error->message);

  if (text) PrintSourceContext(text, error->pos, error->length, 1);

  fprintf(stderr, "%s\n", ANSINormal);
  if(text) free(text);
}
