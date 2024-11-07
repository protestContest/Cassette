#include "univ/hash.h"

u32 Hash(void *data, u32 size)
{
  return AppendHash(EmptyHash, data, size);
}

u32 AppendHash(u32 hash, void *data, u32 size)
{
  u32 i;
  u8 *bytes = (u8*)data;
  for (i = 0; i < size; i++) {
    hash = ((hash << 5) + hash) + bytes[i];
  }
  return hash;
}

u32 FoldHash(u32 hash, i32 size)
{
  u32 mask = 0xFFFFFFFF >> (32 - size);
  return (hash ^ ((hash & ~mask) >> size)) & mask;
}
