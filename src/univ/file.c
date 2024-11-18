#include "univ/file.h"
#include "univ/str.h"
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

static int OnlyFiles(const struct dirent *entry)
{
  return entry->d_type == DT_REG && entry->d_name[0] != '.';
}

static int OnlyFolders(const struct dirent *entry)
{
  return entry->d_type == DT_DIR && entry->d_name[0] != '.';
}

static char *FileExt(char *path)
{
  return strrchr(path, '.');
}

FileList ListFiles(char *path, char *ext, FileList list)
{
  struct dirent **filenames;
  struct dirent **foldernames;
  i32 num_files, num_folders, i;

  if (!list) InitVec(list);

  num_files = scandir(path, &filenames, OnlyFiles, 0);
  num_folders = scandir(path, &foldernames, OnlyFolders, 0);

  if (num_files <= 0 && num_folders <= 0) return list;

  if (ext) {
    for (i = 0; i < num_files; i++) {
      char *fext = FileExt(filenames[i]->d_name);
      if (!StrEq(ext, fext)) {
        free(filenames[i]);
        filenames[i] = 0;
      }
    }
  }

  for (i = 0; i < num_files; i++) {
    if (filenames[i]) {
      char **filename = JoinStr(path, filenames[i]->d_name, '/');
      VecPush(list, filename);
      free(filenames[i]);
    }
  }
  free(filenames);

  for (i = 0; i < num_folders; i++) {
    char **foldername = JoinStr(path, foldernames[i]->d_name, '/');
    list = ListFiles(*foldername, ext, list);
    DisposeHandle(foldername);
    free(foldernames[i]);
  }
  free(foldernames);

  return list;
}

void FreeFileList(FileList list)
{
  u32 i;
  for (i = 0; i < VecCount(list); i++) DisposeHandle(VecAt(list, i));
  FreeVec(list);
}

char **ReadFile(char *path)
{
  int file;
  u32 size;
  char **data;

  file = open(path, O_RDWR, 0);
  if (file < 0) return 0;
  size = lseek(file, 0, 2);
  lseek(file, 0, 0);
  data = (char**)NewHandle(size + 1);
  read(file, *data, size);
  (*data)[size] = 0;
  close(file);
  return data;
}

i32 WriteFile(u8 *data, u32 size, char *path)
{
  int file;
  i32 written;
  u32 mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  file = open(path, O_RDWR | O_CREAT | O_TRUNC, mode);
  if (file < 0) return -1;
  written = write(file, data, size);
  close(file);
  return written;
}
