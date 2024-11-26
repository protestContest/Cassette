#include "runtime/error.h"
#include "runtime/mem.h"
#include "univ/file.h"
#include "univ/math.h"
#include "univ/str.h"

Error *NewError(char *message, char *filename, u32 pos, u32 length)
{
  Error *error = malloc(sizeof(Error));
  error->message = NewString(message);
  error->filename = NewString(filename);
  error->pos = pos;
  error->length = length;
  error->stacktrace = 0;
  return error;
}

Error *NewRuntimeError(char *message, u32 pos, u32 link, SourceMap *srcmap)
{

  char *file = GetSourceFile(pos, srcmap);
  u32 srcpos = GetSourcePos(pos, srcmap);
  Error *error = NewError("Runtime error: ^", file, srcpos, 0);
  error->message = FormatString(error->message, message);
  error->stacktrace = BuildStackTrace(link, srcmap);
  return error;
}

void FreeError(Error *error)
{
  if (error->message) free(error->message);
  if (error->filename) free(error->filename);
  if (error->stacktrace) FreeStackTrace(error->stacktrace);
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
    fprintf(stderr, " %*d│ %*.*s",
      linenum_width, linenum+1, len, len, startline);
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

void PrintError(Error *error)
{
  char *text = 0;
  u32 line, col;
  if (error->filename) {
    text = ReadFile(error->filename);
    if (text) {
      line = LineNum(text, error->pos);
      col = ColNum(text, error->pos);
      fprintf(stderr, "%s:%d:%d: ", error->filename, line+1, col+1);
    }
  }
  fprintf(stderr, "%s\n", error->message);

  if (text) PrintSourceContext(text, error->pos, error->length, 1);

  fprintf(stderr, "\n");
  if(text) free(text);

  if (error->stacktrace) PrintStackTrace(error->stacktrace);
}

StackTrace *NewStackTrace(StackTrace *next)
{
  StackTrace *trace = malloc(sizeof(StackTrace));
  trace->filename = 0;
  trace->pos = 0;
  trace->next = next;
  return trace;
}

void FreeStackTrace(StackTrace *st)
{
  while (st) {
    StackTrace *next = st->next;
    free(st);
    st = next;
  }
}

StackTrace *BuildStackTrace(u32 link, SourceMap *srcmap)
{
  StackTrace *trace = NewStackTrace(0);
  StackTrace *item, *st = trace;

  while (link > 0) {
    u32 code_pos = RawInt(StackPeek(StackSize() - link - 1));
    item = NewStackTrace(0);
    trace->next = item;
    item->filename = GetSourceFile(code_pos, srcmap);
    if (item->filename) {
      item->pos = GetSourcePos(code_pos, srcmap);
    } else {
      item->pos = code_pos;
    }
    trace = item;
    link = RawInt(StackPeek(StackSize() - link));
  }
  return st;
}

void PrintStackTrace(StackTrace *st)
{
  u32 colwidth = 0;
  StackTrace *trace = st;

  while (trace) {
    if (trace->filename) {
      char *text = ReadFile(trace->filename);
      if (text) {
        u32 line_num = LineNum(text, trace->pos);
        u32 col = ColNum(text, trace->pos);
        u32 len = snprintf(0, 0, "  %s:%d:%d: ",
            trace->filename, line_num+1, col+1);
        free(text);
        colwidth = Max(colwidth, len);
      }
    }
    trace = trace->next;
  }

  trace = st;
  fprintf(stderr, "Stacktrace:\n");
  while (trace) {
    if (trace->filename) {
      char *text = ReadFile(trace->filename);
      if (text) {
        u32 line = LineNum(text, trace->pos);
        u32 col = ColNum(text, trace->pos);
        u32 j, printed;
        printed = fprintf(stderr, "  %s:%d:%d: ",
            trace->filename, line+1, col+1);
        for (j = 0; j < colwidth - printed; j++) fprintf(stderr, " ");
        PrintSourceLine(text, trace->pos);
        free(text);
      } else {
        fprintf(stderr, "%s@%d\n", trace->filename, trace->pos);
      }
    } else {
      fprintf(stderr, "  (system)@%d\n", trace->pos);
    }
    trace = trace->next;
  }
}

