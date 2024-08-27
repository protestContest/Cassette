#include "univ/hash.h"

u32 Hash(void *data, u32 size)
{
  u32 i;
  u32 hash = 5381;
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
