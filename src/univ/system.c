#include "univ.h"
#include <stdlib.h>
#include <stdio.h>

#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>


static void *handles[8] = {NULL};

Handle NewHandle(u32 size)
{
  u32 i;
  for (i = 0; i < ArrayCount(handles); i++) {
    if (handles[i] == NULL) {
      handles[i] = malloc(size);
      return &handles[i];
    }
  }

  return NULL;
}

void SetHandleSize(Handle handle, u32 size)
{
  *handle = realloc(*handle, size);
  printf("%p\n", *handle);
}

void DisposeHandle(Handle handle)
{
  if (handle && *handle) free(*handle);
  *handle = NULL;
}

void Copy(void *src, void *dst, u32 size)
{
  u32 i;
  u32 words = size / sizeof(u32);
  u32 rem = size % sizeof(u32);
  for (i = 0; i < words; i++) ((u32*)dst)[i] = ((u32*)src)[i];
  for (i = 0; i < rem; i++) ((u8*)((u32*)dst + words))[i] = ((u8*)((u32*)src + words))[i];
}

void Exit(void)
{
  exit(0);
}

void Alert(char *message)
{
  printf("%s\n", message);
}

char *ReadFile(char *path)
{
  mode_t mode = S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR;
  int file;
  u32 size, pos;
  char *data;

  file = open(path, O_RDWR|O_CREAT, mode);
  if (file < 0) return NULL;

  pos = lseek(file, 0, 1);
  size = lseek(file, 0, 2);
  lseek(file, pos, 0);

  data = malloc(size + 1);
  read(file, data, size);
  data[size] = '\0';
  return data;
}
