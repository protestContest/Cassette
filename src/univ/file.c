#include "file.h"
#include "system.h"
#include "str.h"
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

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

char *FileExt(char *name)
{
  char *ext = name + StrLen(name);
  while (ext > name && *(ext-1) != '.') ext--;
  return ext;
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
