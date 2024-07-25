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
  error->message = message;
  error->file = file;
  error->pos = pos;
  error->length = length;
  return error;
}

void PrintSourceContext(char *text, u32 pos, u32 length, u32 context)
{
  u32 line = LineNum(text, pos);
  u32 col = ColNum(text, pos);
  u32 start = (line > context) ? line - context : 0;
  u32 i, len;
  u32 numlines = 0;
  u32 linecol = 0;
  char *cur = text;
  char *end;

  while (*cur) {
    numlines++;
    cur = LineEnd(cur-text, text);
  }
  linecol = NumDigits(numlines, 10);
  cur = text;

  for (i = 0; i < start; i++) cur = LineEnd(cur - text, text);
  for (i = start; i < line; i++) {
    end = LineEnd(cur-text, text);
    len = end - cur;
    fprintf(stderr, " % *d│ %*.*s", linecol, i+1, len, len, cur);
    cur = end;
  }
  end = LineEnd(cur-text, text);
  fprintf(stderr, ">% *d│ %*.*s", linecol, line+1, col, col, cur);
  cur += col;

  fprintf(stderr, "%s%*.*s%s", ANSIUnderline, length, length, cur, ANSINoUnderline);

  cur += length;
  len = end - cur;
  fprintf(stderr, "%*.*s", len, len, cur);
  cur = end;
  for (i = 0; i < context && *end; i++) {
    end = LineEnd(cur-text, text);
    len = end - cur;
    fprintf(stderr, " % *d│ %*.*s", linecol, line+i+2, len, len, cur);
    cur = end;
  }
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
