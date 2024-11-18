#include "univ/vec.h"

Handle MakeVec(u32 count, u32 itemSize)
{
  u32 size = count*itemSize + sizeof(Vec);
  Handle h = NewHandle(size);
  Vec **v = (Vec**)h;
  (*v)->count = 0;
  return h;
}

void DoVecDelete(Handle vec, u32 index, u32 itemSize)
{
  u32 size;
  ByteVec bv = (ByteVec)vec;
  u8 *data;
  if (index >= VecCount(vec)) return;
  data = VecData(bv) + index*itemSize;
  size = (VecCount(vec) - index - 1)*itemSize;
  Copy(data + itemSize, data, size);
  VecCount(vec)--;
}
