#include "file.h"
#include "memory.h"
#include "math.h"
#include "system.h"
#include "string.h"
#include <dirent.h>
#include <unistd.h>

static void ListFilesRec(char *path, FileSet *files)
{
  DIR *d;
  struct dirent *entry;
  d = opendir(path);
  if (d) {
    while ((entry = readdir(d)) != NULL) {
      if (entry->d_name[0] == '.') continue;

      if (entry->d_type == DT_REG) {
        char *entry_name = JoinPath(path, entry->d_name);
        if (files->count >= files->capacity) {
          files->capacity = Max(2, 2*files->capacity);
          Reallocate(files->files, files->capacity);
        }
        files->files[files->count++] = entry_name;
      } else if (entry->d_type == DT_DIR) {
        char *sub_path = JoinPath(path, entry->d_name);
        ListFilesRec(sub_path, files);
        Free(sub_path);
      }
    }
  }
}

FileSet ListFiles(char *path)
{
  FileSet set = {0, 0, NULL};
  ListFilesRec(path, &set);
  return set;
}

char *JoinPath(char *path1, char *path2)
{
  u32 len1 = StrLen(path1);
  u32 len2 = StrLen(path2);
  if (path1[len1 - 1] == '/') len1--;
  char *joined = Allocate(len1 + len2 + 2);

  Copy(path1, joined, len1);
  joined[len1] = '/';
  Copy(path2, joined + len1 + 1, len2);
  joined[len1 + len2 + 1] = '\0';
  return joined;
}

u32 FileSize(u32 file)
{
  u32 pos = lseek(file, 0, 1);
  u32 size = lseek(file, 0, 2);
  lseek(file, pos, 0);
  return size;
}

void *ReadFile(char *path)
{
  int file = Open(path);
  if (file < 0) return NULL;

  u32 size = FileSize(file);
  u8 *data = Allocate(size + 1);
  Read(file, data, size);
  data[size] = '\0';
  return data;
}

bool WriteFile(char *path, void *data, u32 size)
{
  int file = Open(path);
  if (file < 0) return false;

  return Write(file, data, size);
}

bool SniffFile(char *path, u32 tag)
{
  int file = Open(path);
  if (file < 0) return NULL;

  u32 data;
  Read(file, &data, sizeof(data));
  return tag == data;
}
