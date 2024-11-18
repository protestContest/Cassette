#ifndef __APPLE__
#include "univ/handle.h"

static void *handles[1024];
static u32 nHandles = 0;

Handle NewHandle(u32 size)
{
  u32 i;
  Handle h = 0;
  u32 *p;
  for (i = 0; i < nHandles; i++) {
    if (handles[i] == 0) {
      h = &handles[i];
      break;
    }
  }
  if (!h) h = &handles[nHandles++];

  p = malloc(size + sizeof(u32));
  *p = size;
  *h = p+1;
  return h;
}

void DisposeHandle(Handle h)
{
  u32 *p = (u32*)(*h) - 1;
  *h = 0;
  free(p);
}

u32 GetHandleSize(Handle h)
{
  u32 *p = (u32*)(*h) - 1;
  return *p;
}

void SetHandleSize(Handle h, u32 size)
{
  u32 *p = (u32*)(*h) - 1;
  p = realloc(p, sizeof(u32)+size);
  *h = p;
}

#endif
