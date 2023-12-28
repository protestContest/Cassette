#pragma once
#include "mem/mem.h"

typedef struct Frame {
  struct Frame *parent;
  u32 count;
  Val *items;
} Frame;

Frame *ExtendFrame(Frame *parent, u32 size);
Frame *PopFrame(Frame *frame);
void FrameSet(Frame *frame, u32 index, Val value);
i32 FrameFind(Frame *frame, Val value);
