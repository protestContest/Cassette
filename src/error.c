#include "error.h"
#include <univ/str.h>

val MakeError(val msg, val positions)
{
  val err = Tuple(3);
  TupleSet(err, 0, 0);
  TupleSet(err, 1, msg);
  TupleSet(err, 2, positions);
  return err;
}

val PrefixError(val error, char *prefix)
{
  TupleSet(error, 1, Pair(Binary(prefix), ErrorMsg(error)));
  return error;
}

void PrintError(val error, char *source)
{
  char *msg = FormatVal(ErrorMsg(error), 0);
  u32 start_pos = ErrorStart(error);
  u32 end_pos = ErrorEnd(error);
  u32 pos = LineStart(start_pos, source) - source;
  u32 end = LineEnd(end_pos, source) - source;

  fprintf(stderr, "%s%s\n", ANSIRed, msg);

  fprintf(stderr, "%d-%d\n", start_pos, end_pos);

  while (pos < end) {
    u32 line_end = LineEnd(pos, source) - source;
    u32 err_start = start_pos > pos ? start_pos : pos;
    u32 err_end = end_pos < line_end ? end_pos :  line_end;
    u32 len;

    fprintf(stderr, "%3d│ ", LineNum(source, pos) + 1);
    len = err_start - pos;
    fprintf(stderr, "%*.*s", len, len, source + pos);
    len = err_end - err_start;
    fprintf(stderr, "%s%*.*s%s", ANSIUnderline, len, len, source + err_start, ANSINoUnderline);
    len = line_end - err_end;
    fprintf(stderr, "%*.*s", len, len, source + err_end);

    pos = line_end;
  }

/*
  u32 start_line = LineNum(source, start);
  u32 end_line = LineNum(source, end);
  u32 line_num;

  if (start_line > 0) start_line--;

  for (line_num = start_line; line_num <= end_line; line_num++) {
    char *line = LineStart(cur, source);
    char *line_end = LineEnd(cur, source);
    u32 len;
    fprintf(stderr, "%3d| ", line_num);

    if (line < source + start) {
      char *prefix_end = (line_end < source + start) ? line_end : source + start;
      len = prefix_end - line;
      fprintf(stderr, "%*.*s", len, len, line);
      line += len;
    }
    len = source + end - line;
    fprintf(stderr, "%s%*.*s%s", ANSIUnderline, len, len, line, ANSINoUnderline);
    if (line_end > source + end) {
      len = line_end - (source + end);
      fprintf(stderr, "%*.*s", len, len, source + end);
      line += len;
    }
    fprintf(stderr, "\n");
  }
*/

  free(msg);
/*




  i32 nodeStart = NodeStart(node);
  i32 nodeLen = NodeEnd(node) - nodeStart;
  i32 col = 0, i;
  i32 line = 0;
  if (!node) return;

  if (prefix) {
    fprintf(stderr, "%s%s: %s\n", ANSIRed, prefix, ErrorMsg(node));
  } else {
    fprintf(stderr, "%s%s\n", ANSIRed, ErrorMsg(node));
  }
  if (source) {
    char *start = source + nodeStart;
    char *end = source + nodeStart;
    while (start > source && !IsNewline(start[-1])) {
      col++;
      start--;
    }
    while (*end && !IsNewline(*end)) end++;
    for (i = 0; i < nodeStart; i++) {
      if (IsNewline(source[i])) line++;
    }
    fprintf(stderr, "%3d| %*.*s\n",
        line+1, (i32)(end-start), (i32)(end-start), start);
    fprintf(stderr, "     ");
    for (i = 0; i < col; i++) fprintf(stderr, " ");
    for (i = 0; i < nodeLen; i++) fprintf(stderr, "^");
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "%s", ANSINormal);
  */
}
