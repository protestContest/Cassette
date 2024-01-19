#pragma once

typedef struct Frame {
  struct Frame *parent;
  u32 count;
  Val *items;
} Frame;

Frame *CompileEnv(u32 num_modules);
Frame *ExtendFrame(Frame *parent, u32 size);
Frame *PopFrame(Frame *frame);
void FreeEnv(Frame *env);
void FrameSet(Frame *frame, u32 index, Val value);
i32 FrameFind(Frame *frame, Val value);
i32 FrameNum(Frame *frame, Val value);
i32 ExportsFrame(Frame *frame);
