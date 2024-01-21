#include "stacktrace.h"
#include "chunk.h"
#include "univ/str.h"
#include "univ/system.h"

StackTraceItem *NewStackTraceItem(char *filename, u32 position, StackTraceItem *prev)
{
  StackTraceItem *item = Alloc(sizeof(StackTraceItem));
  item->filename = CopyStr(filename, StrLen(filename));
  item->position = position;
  item->prev = prev;
  return item;
}

void FreeStackTrace(StackTraceItem *trace)
{
  while(trace) {
    StackTraceItem *prev = trace->prev;
    Free(trace->filename);
    Free(trace);
    trace = prev;
  }
}

StackTraceItem *StackTrace(VM *vm)
{
  u32 link = vm->link;
  u32 file_pos;
  char *filename;
  StackTraceItem *trace = 0;

  while (link > 0 && vm->stack.count > 2) {
    u32 index = RawInt(VecRef(&vm->stack, link - 2));
    /* this hack depends on the compiler always linking to a point just after an
    apply op */
    index -= 2;
    file_pos = SourcePosAt(index, vm->chunk);
    filename = ChunkFileAt(index, vm->chunk);
    trace = NewStackTraceItem(filename, file_pos, trace);
    link = RawInt(VecRef(&vm->stack, link - 1));
  }

  return trace;
}
