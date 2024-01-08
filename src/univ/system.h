#pragma once

/* Wrappers for system and memory functions */

#include <assert.h>

#define Assert(expr)    assert(expr)

void *Alloc(u32 size);
void *Realloc(void *ptr, u32 size);
void Free(void *ptr);
void Copy(void *src, void *dst, u32 size);
char *GetEnv(char *name);
u32 Time(void);
u32 Ticks(void);
void Exit(void);
