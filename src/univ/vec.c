#include "univ/vec.h"

void *ResizeVec(void *vec, u32 num_items, u32 item_size)
{
  u32 double_cap = vec ? 2*VecCapacity(vec) : 4;
  u32 min_cap = VecCount(vec) + num_items;
  u32 new_cap = Max(double_cap, min_cap);
  void *raw_vec = vec ? RawVec(vec) : 0;
  u32 *new_vec = (u32*)realloc(raw_vec, item_size*new_cap + sizeof(u32)*2);
  assert(new_vec);
  new_vec[0] = new_cap;
  if (!vec) new_vec[1] = 0;
  return new_vec + 2;
}
