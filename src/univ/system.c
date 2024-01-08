#include "system.h"
#include <stdlib.h>
#include <time.h>

void *Alloc(u32 size)
{
  return malloc(size);
}

void *Realloc(void *ptr, u32 size)
{
  return realloc(ptr, size);
}

void Free(void *ptr)
{
  free(ptr);
}

void Copy(void *src, void *dst, u32 size)
{
  u32 i;
  u32 words = size / sizeof(u32);
  u32 rem = size % sizeof(u32);
  for (i = 0; i < words; i++) ((u32*)dst)[i] = ((u32*)src)[i];
  for (i = 0; i < rem; i++) ((u8*)((u32*)dst + words))[i] = ((u8*)((u32*)src + words))[i];
}

char *GetEnv(char *name)
{
  return getenv(name);
}

u32 Time(void)
{
  return time(0);
}

u32 Ticks(void)
{
  return clock() / (CLOCKS_PER_SEC / 1000);
}

void Exit(void)
{
  exit(0);
}
