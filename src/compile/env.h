#pragma once
#include "univ/vec.h"

typedef struct Frame {
  struct Frame *parent;
  IntVec items;
} Frame;

Frame *ExtendFrame(Frame *parent, u32 size);
Frame *PopFrame(Frame *frame);
void FreeEnv(Frame *env);
void FrameSet(Frame *frame, u32 index, u32 name);
i32 FrameFind(Frame *frame, u32 name);
void PrintEnv(Frame *frame);
