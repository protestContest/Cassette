#include "memory.h"
#include <stdlib.h>

void *Allocate(u32 size)
{
  return malloc(size);
}

void *Reallocate(void *ptr, u32 size)
{
  return realloc(ptr, size);
}

void Free(void *ptr)
{
  free(ptr);
}

void Copy(void *src, void *dst, u32 size)
{
  u32 words = size / sizeof(u32);
  for (u32 i = 0; i < words; i++) ((u32*)dst)[i] = ((u32*)src)[i];
  u32 rem = size % sizeof(u32);
  for (u32 i = 0; i < rem; i++) ((u8*)((u32*)dst + words))[i] = ((u8*)((u32*)src + words))[i];
}
