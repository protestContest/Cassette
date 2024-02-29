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
    frame->items[i] = 0;
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

void FreeEnv(Frame *env)
{
  while (env != 0) env = PopFrame(env);
}

void FrameSet(Frame *frame, u32 index, Val value)
{
  Assert(index < frame->count);
  frame->items[index] = value;
}

i32 FrameFind(Frame *frame, Val value)
{
  i32 index;
  for (index = (i32)frame->count - 1; index >= 0; index--) {
    if (frame->items[index] == value) return index;
  }

  if (frame->parent == 0) return -1;
  index = FrameFind(frame->parent, value);
  if (index < 0) return index;
  return frame->count + index;
}

i32 FrameNum(Frame *frame, Val value)
{
  while (frame != 0) {
    i32 i;
    for (i = 0; i < (i32)frame->count; i++) {
      if (frame->items[i] == value) {
        i32 num = 0;
        while (frame->parent != 0) {
          frame = frame->parent;
          num++;
        }
        return num;
      }
    }
    frame = frame->parent;
  }
  return -1;
}

i32 ExportsFrame(Frame *frame)
{
  i32 num = 0;
  if (frame == 0 || frame->parent == 0) return -1;
  while (frame->parent->parent != 0) {
    num += frame->count;
    frame = frame->parent;
  }
  return num;
}
