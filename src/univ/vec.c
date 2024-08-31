#include "univ/vec.h"
#include "univ/str.h"

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

void VecDelete(void *vec, u32 index, u32 item_size)
{
  u32 size = VecCount(vec)*item_size;
  u8 *data = (u8*)vec;
  if (!vec) return;
  Copy(data + (index+1)*item_size, data + index*item_size, size);
  RawVecCount(vec)--;
}

void *DoVecCat(void *a, void *b, u32 item_size)
{
  if (!a) return b;
  if (!b) return a;
  a = ResizeVec(a, VecCount(b), item_size);
  Copy((u8*)b, (u8*)a + item_size*VecCount(a), item_size*VecCount(b));
  RawVecCount(a) += VecCount(b);
  return a;
}

