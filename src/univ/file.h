#pragma once

typedef struct {
  u32 capacity;
  u32 count;
  char **files;
} FileSet;

FileSet ListFiles(char *path);
char *JoinPath(char *path1, char *path2);
u32 FileSize(u32 file);
void *ReadFile(char *path);
bool WriteFile(char *path, void *data, u32 length);
bool SniffFile(char *path, u32 tag);
