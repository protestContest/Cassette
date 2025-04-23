#include "univ/vec.h"
#include "univ/str.h"

void *ResizeVec(void *vec, u32 numItems, u32 itemSize)
{
  u32 doubleCap = vec ? 2*VecCapacity(vec) : 4;
  u32 minCap = VecCount(vec) + numItems;
  u32 newCap = Max(doubleCap, minCap);
  void *rawVec = vec ? RawVec(vec) : 0;
  u32 *newVec = (u32*)realloc(rawVec, itemSize*newCap + sizeof(u32)*2);
  assert(newVec);
  newVec[0] = newCap;
  if (!vec) newVec[1] = 0;
  return newVec + 2;
}

void DoVecDelete(u8 *vec, u32 index, u32 itemSize)
{
  u32 count = VecCount(vec) - index - 1;
  Copy(vec + index + 1, vec + index, count * itemSize);
}
