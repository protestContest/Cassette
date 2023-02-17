#include "vec.h"
#include "platform/error.h"

u32 *NewRawVec(u32 item_size, u32 max_items)
{
  u32 *raw_vec = Allocate(item_size*(max_items) + 2*sizeof(u32));
  Assert(raw_vec != NULL);
  raw_vec[0] = max_items;
  raw_vec[1] = 0;
  return raw_vec;
}

void *ResizeVec(void *vec, u32 num_items, u32 item_size)
{
  u32 double_capacity = vec ? 2*VecCapacity(vec) : 4;
  u32 min_required = VecCount(vec) + num_items;
  u32 new_capacity = (double_capacity < min_required) ? min_required : double_capacity;

  u32 *new_raw_vec = (u32 *)Reallocate(vec ? RawVec(vec) : 0, item_size*new_capacity + sizeof(u32)*2);
  if (!new_raw_vec) Fatal("Out of memory");

  new_raw_vec[0] = new_capacity;
  if (!vec) new_raw_vec[1] = 0;
  return new_raw_vec + 2;
}

void *AppendVec(void *dst, void *src, u32 item_size)
{
  if (src == NULL) return dst;

  dst = ResizeVec(dst, VecCount(src), item_size);
  MemCopy(dst + item_size*VecCount(dst), src, item_size*VecCount(src));
  RawVecCount(dst) = VecCount(dst) + VecCount(src);
  return dst;
}
