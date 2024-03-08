#pragma once

typedef struct Frame {
  struct Frame *parent;
  u32 *items;
} Frame;

Frame *ExtendFrame(Frame *parent, u32 size);
Frame *PopFrame(Frame *frame);
void FreeEnv(Frame *env);
void FrameSet(Frame *frame, u32 index, u32 name);
i32 FrameFind(Frame *frame, u32 name);
void PrintEnv(Frame *frame);
