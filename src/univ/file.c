#include "file.h"
#include "system.h"
#include "str.h"
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

i32 Read(i32 file, void *buf, u32 size)
{
  return read(file, buf, size);
}

i32 Write(i32 file, void *buf, u32 size)
{
  return write(file, buf, size);
}

int Open(char *path)
{
  return open(path, O_RDWR, 0);
}

void Close(int file)
{
  close(file);
}

void Truncate(int file)
{
  ftruncate(file, 0);
}

int CreateOrOpen(char *path)
{
  mode_t mode = S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR; /* unix permission 0644 */
  return open(path, O_RDWR | O_CREAT, mode);
}

u32 FileSize(char *filename)
{
  u32 size;
  int file = Open(filename);
  if (file < 0) return 0;
  size = lseek(file, 0, 2);
  Close(file);
  return size;
}

char *ReadFile(char *path)
{
  int file;
  u32 size;
  char *data;

  file = Open(path);
  if (file < 0) return 0;

  size = lseek(file, 0, 2);
  lseek(file, 0, 0);

  data = Alloc(size + 1);
  read(file, data, size);
  data[size] = 0;
  Close(file);
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

void WriteInt(u32 n, u8 *data)
{
  data[0] = (n >> 24) & 0xFF;
  data[1] = (n >> 16) & 0xFF;
  data[2] = (n >> 8) & 0xFF;
  data[3] = n & 0xFF;
}

u32 ReadInt(u8 *data)
{
  return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

void WriteShort(u16 n, u8 *data)
{
  data[0] = (n >> 8) & 0xFF;
  data[1] = n & 0xFF;
}

u16 ReadShort(u8 *data)
{
  return (data[0] << 8) | data[1];
}
