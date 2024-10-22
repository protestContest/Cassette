#include "univ/vec.h"

void *ResizeVec(void *vec, u32 num_items, u32 item_size)
{
  u32 double_capacity = vec ? 2*VecCapacity(vec) : 4;
  u32 min_required = VecCount(vec) + num_items;
  u32 new_capacity = (double_capacity < min_required) ? min_required : double_capacity;

  u32 *new_raw_buffer = (u32 *)realloc(vec ? RawVec(vec) : 0, item_size*new_capacity + sizeof(u32)*2);
  assert(new_raw_buffer != NULL);

  new_raw_buffer[0] = new_capacity;
  if (!vec) new_raw_buffer[1] = 0;

  return new_raw_buffer + 2;
}
