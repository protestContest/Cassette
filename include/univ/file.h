#pragma once
#include <libgen.h>

#define DirName(path)   dirname(path)

typedef struct {
  u32 count;
  char **filenames;
} FileList;

FileList *ListFiles(char *path, char *ext, FileList *list);
void FreeFileList(FileList *list);
char *ReadFile(char *path);
i32 WriteFile(u8 *data, u32 size, char *path);
