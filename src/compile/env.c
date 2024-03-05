#include "env.h"
#include "univ/system.h"

Frame *ExtendFrame(Frame *parent, u32 size)
{
  u32 i;
  Frame *frame = Alloc(sizeof(Frame));
  frame->parent = parent;
  InitVec(&frame->items, sizeof(u32), size);
  for (i = 0; i < size; i++) {
    IntVecPush(&frame->items, 0);
  }
  return frame;
}

Frame *PopFrame(Frame *frame)
{
  Frame *parent = frame->parent;
  DestroyVec(&frame->items);
  Free(frame);
  return parent;
}

void FreeEnv(Frame *env)
{
  while (env != 0) env = PopFrame(env);
}

void FrameSet(Frame *frame, u32 index, u32 name)
{
  Assert(index < frame->items.count);
  VecRef(&frame->items, index) = name;
}

i32 FrameFind(Frame *frame, u32 var)
{
  i32 index;
  for (index = (i32)frame->items.count - 1; index >= 0; index--) {
    u32 item = VecRef(&frame->items, index);
    if (item == var) return index;
  }

  if (frame->parent == 0) return -1;
  index = FrameFind(frame->parent, var);
  if (index < 0) return index;
  return frame->items.count + index;
}
