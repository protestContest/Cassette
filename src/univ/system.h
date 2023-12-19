#pragma once
#include "vec.h"
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
char *GetEnv(char *name);
char *FileExt(char *name);
bool DirExists(char *path);
void DirContents(char *path, char *ext, ObjVec *contents);
u32 Time(void);
u32 Ticks(void);
void Exit(void);
