#include "vec.h"

u32 *NewRawVec(u32 item_size, u32 max_items)
{
  u32 *raw_vec = malloc(item_size*(max_items) + 2*sizeof(u32));
  assert(raw_vec != NULL);
  raw_vec[0] = max_items;
  raw_vec[1] = 0;
  return raw_vec;
}

void *ResizeVec(void *vec, u32 num_items, u32 item_size)
{
  u32 double_capacity = vec ? 2*VecCapacity(vec) : 4;
  u32 min_required = VecCount(vec) + num_items;
  u32 new_capacity = (double_capacity < min_required) ? min_required : double_capacity;

  u32 *new_raw_vec = (u32 *)realloc(vec ? RawVec(vec) : 0, item_size*new_capacity + sizeof(u32)*2);
  if (!new_raw_vec) Fatal("Out of memory");

  new_raw_vec[0] = new_capacity;
  if (!vec) new_raw_vec[1] = 0;
  return new_raw_vec + 2;
}

void *CopyVec(void *src)
{
  if (src == NULL) return NULL;
  u32 *raw_vec = (u32 *)malloc(VecCapacity(src) + sizeof(u32)*2);
  memcpy(raw_vec, RawVec(src), VecCapacity(src) + sizeof(u32)*2);
  return raw_vec + 2;
}
