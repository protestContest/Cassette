#include "env.h"
#include "vec.h"

Frame *NewFrame(Frame *parent)
{
  Frame *frame = malloc(sizeof(Frame));
  frame->keys = NULL;
  frame->values = NULL;
  frame->parent = parent;
  return frame;
}

void FreeFrame(Frame *frame)
{
  FreeVec(frame->keys);
  FreeVec(frame->values);
  free(frame);
}
