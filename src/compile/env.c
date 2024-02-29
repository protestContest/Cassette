#include "env.h"
#include "univ/system.h"

Frame *ExtendFrame(Frame *parent, u32 size)
{
  u32 i;
  Frame *frame = Alloc(sizeof(Frame));
  frame->parent = parent;
  InitVec(&frame->items, sizeof(FrameItem*), size);
  for (i = 0; i < size; i++) {
    FrameItem *item = Alloc(sizeof(FrameItem));
    item->type = AnyType;
    item->name = 0;
    ObjVecPush(&frame->items, item);
  }
  return frame;
}

Frame *PopFrame(Frame *frame)
{
  u32 i;
  Frame *parent = frame->parent;
  for (i = 0; i < frame->items.count; i++) {
    Free(VecRef(&frame->items, i));
  }
  DestroyVec(&frame->items);
  Free(frame);
  return parent;
}

void FreeEnv(Frame *env)
{
  while (env != 0) env = PopFrame(env);
}

void FrameSet(Frame *frame, u32 index, ValType type, u32 name)
{
  FrameItem *item;
  Assert(index < frame->items.count);
  item = VecRef(&frame->items, index);
  item->type = type;
  item->name = name;
}

i32 FrameFind(Frame *frame, u32 var)
{
  i32 index;
  for (index = (i32)frame->items.count - 1; index >= 0; index--) {
    FrameItem *item = VecRef(&frame->items, index);
    if (item->name == var) return index;
  }

  if (frame->parent == 0) return -1;
  index = FrameFind(frame->parent, var);
  if (index < 0) return index;
  return frame->items.count + index;
}

ValType FrameFindType(Frame *frame, u32 index)
{
  if (!frame) return AnyType;

  if (index >= frame->items.count) {
    return FrameFindType(frame->parent, index - frame->items.count);
  } else {
    FrameItem *item = VecRef(&frame->items, index);
    return item->type;
  }
}
