#include "env.h"
#include <univ.h>

Frame *ExtendFrame(Frame *parent, u32 size)
{
  u32 i;
  Frame *frame = malloc(sizeof(Frame));
  frame->parent = parent;
  frame->items = NewVec(u32, size);
  for (i = 0; i < size; i++) {
    VecPush(frame->items, 0);
  }
  return frame;
}

Frame *PopFrame(Frame *frame)
{
  Frame *parent = frame->parent;
  FreeVec(frame->items);
  free(frame);
  return parent;
}

void FreeEnv(Frame *env)
{
  while (env != 0) env = PopFrame(env);
}

void FrameSet(Frame *frame, u32 index, u32 name)
{
  assert(index < VecCount(frame->items));
  frame->items[index] = name;
}

i32 FrameFind(Frame *frame, u32 var)
{
  i32 index;
  for (index = (i32)VecCount(frame->items) - 1; index >= 0; index--) {
    u32 item = frame->items[index];
    if (item == var) return index;
  }

  if (frame->parent == 0) return -1;
  index = FrameFind(frame->parent, var);
  if (index < 0) return index;
  return VecCount(frame->items) + index;
}

void PrintEnv(Frame *frame)
{
  while (frame) {
    u32 i;
    printf("- ");
    for (i = 0; i < VecCount(frame->items); i++) {
      u32 var = frame->items[i];
      printf("%s ", SymbolName(var));
    }
    printf("\n");
    frame = frame->parent;
  }
}

