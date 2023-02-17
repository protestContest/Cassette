#include "allocate.h"
#include <stdlib.h>
#include <string.h>

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

void MemCopy(void *dst, void *src, u32 size)
{
  memcpy(dst, src, size);
}
