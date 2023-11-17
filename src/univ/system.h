#pragma once
#include <assert.h>

#define Assert(expr)    assert(expr)

void *Alloc(u32 size);
void *Realloc(void *ptr, u32 size);
void Free(void *ptr);
void Copy(void *src, void *dst, u32 size);
i32 Read(i32 file, void *buf, u32 size);
i32 Write(i32 file, void *buf, u32 size);
int Open(char *path);
int CreateOrOpen(char *path);
u32 FileSize(int file);
char *ReadFile(char *path);
u32 Ticks(void);
