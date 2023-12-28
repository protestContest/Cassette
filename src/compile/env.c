#include "env.h"
#include "univ/system.h"

Frame *ExtendFrame(Frame *parent, u32 size)
{
  u32 i;
  Frame *frame = Alloc(sizeof(Frame));
  frame->parent = parent;
  frame->count = size;
  frame->items = Alloc(size * sizeof(Val));
  for (i = 0; i < size; i++) {
    frame->items[i] = Nil;
  }
  return frame;
}

Frame *PopFrame(Frame *frame)
{
  Frame *parent = frame->parent;
  Free(frame->items);
  Free(frame);
  return parent;
}

void FrameSet(Frame *frame, u32 index, Val value)
{
  Assert(index < frame->count);
  frame->items[index] = value;
}

i32 FrameFind(Frame *frame, Val value)
{
  i32 index;
  for (index = 0; index < (i32)frame->count; index++) {
    if (frame->items[index] == value) return index;
  }

  if (frame->parent == 0) return -1;
  index = FrameFind(frame->parent, value);
  if (index < 0) return index;
  return frame->count + index;
}
