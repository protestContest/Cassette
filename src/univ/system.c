#include "system.h"
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

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

i32 Read(i32 file, void *buf, u32 size)
{
  return read(file, buf, size);
}

i32 Write(i32 file, void *buf, u32 size)
{
  return write(file, buf, size);
}

void Copy(void *src, void *dst, u32 size)
{
  u32 i;
  u32 words = size / sizeof(u32);
  u32 rem = size % sizeof(u32);
  for (i = 0; i < words; i++) ((u32*)dst)[i] = ((u32*)src)[i];
  for (i = 0; i < rem; i++) ((u8*)((u32*)dst + words))[i] = ((u8*)((u32*)src + words))[i];
}

int Open(char *path)
{
  mode_t mode = S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR; /* unix permission 0644 */
  return open(path, O_RDWR, mode);
}

u32 FileSize(int file)
{
  u32 pos = lseek(file, 0, 1);
  u32 size = lseek(file, 0, 2);
  lseek(file, pos, 0);
  return size;
}

char *ReadFile(char *path)
{
  int file;
  u32 size;
  char *data;

  file = Open(path);
  if (file < 0) return 0;

  size = FileSize(file);

  data = Alloc(size + 1);
  read(file, data, size);
  data[size] = 0;
  return data;
}

u32 Ticks(void)
{
  return clock();
}
