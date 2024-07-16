#include "error.h"
#include <univ/str.h>
#include <univ/symbol.h>

val MakeError(val msg, val positions)
{
  val err = Tuple(3);
  TupleSet(err, 0, SymVal(Symbol("error")));
  TupleSet(err, 1, msg);
  TupleSet(err, 2, positions);
  return err;
}

val PrefixError(val error, char *prefix)
{
  PrintError(error, 0);
  TupleSet(error, 1, Pair(Binary(prefix), Pair(ErrorMsg(error), 0)));
  return error;
}

void PrintError(val error, char *source)
{
  val msg = FormatVal(ErrorMsg(error));
  u32 msg_len = BinaryLength(msg);
  u32 start_pos, end_pos, pos, end;

  fprintf(stderr, "%s%*.*s\n", ANSIRed, msg_len, msg_len, BinaryData(msg));
  if (!source) return;

  start_pos = ErrorStart(error);
  end_pos = ErrorEnd(error);
  pos = LineStart(start_pos, source) - source;
  end = LineEnd(end_pos, source) - source;

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
}
