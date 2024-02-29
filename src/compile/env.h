#pragma once
#include "value.h"
#include "univ/vec.h"

typedef struct {
  ValType type;
  u32 name;
} FrameItem;

typedef struct Frame {
  struct Frame *parent;
  ObjVec items;
} Frame;

Frame *ExtendFrame(Frame *parent, u32 size);
Frame *PopFrame(Frame *frame);
void FreeEnv(Frame *env);
void FrameSet(Frame *frame, u32 index, ValType type, u32 name);
i32 FrameFind(Frame *frame, u32 name);
ValType FrameFindType(Frame *frame, u32 index);
