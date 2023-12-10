#include "system.h"
#include "str.h"
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

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
  return open(path, O_RDWR, 0);
}

int CreateOrOpen(char *path)
{
  mode_t mode = S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR; /* unix permission 0644 */
  return open(path, O_RDWR | O_CREAT, mode);
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

bool DirExists(char *path)
{
  DIR* dir = opendir(path);
  if (dir) {
      closedir(dir);
      return true;
  } else {
    return false;
  }
}

char *GetEnv(char *name)
{
  return getenv(name);
}

char *FileExt(char *name)
{
  char *ext = name + StrLen(name);
  while (ext > name && *(ext-1) != '.') ext--;
  return ext;
}

void DirContents(char *path, char *ext, ObjVec *contents)
{
  struct dirent *ent;
  DIR *dir = opendir(path);
  if (!dir) return;

  while ((ent = readdir(dir)) != NULL) {
    if (ext == 0 || StrEq(FileExt(ent->d_name), ext)) {
      char *name = JoinStr(path, ent->d_name, '/');
      ObjVecPush(contents, name);
    }
  }
}

u32 Ticks(void)
{
  return clock() / (CLOCKS_PER_SEC / 1000);
}

void Exit(void)
{
  exit(0);
}
