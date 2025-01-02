#pragma once

typedef struct {
  u32 count;
  char **filenames;
} FileList;

FileList *NewFileList(u32 count);
void FreeFileList(FileList *list);

i32 Open(char *path, i32 flags, char **error);
i32 OpenSerial(char *path, i32 speed, i32 opts, char **error);
char *Close(i32 file);
i32 Read(i32 file, char *buf, u32 size, char **error);
i32 Write(i32 file, char *buf, u32 size, char **error);
i32 Seek(i32 file, i32 offset, i32 whence, char **error);
i32 Listen(char *port, char **error);
i32 Accept(i32 sock, char **error);
i32 Connect(char *node, char *port, char **error);

u32 FileSize(char *path);
void *ReadFile(char *path);
char *ReadTextFile(char *path);
i32 WriteFile(void *data, u32 size, char *path);
FileList *ListFiles(char *path, char *ext, FileList *list);
char *FileExt(char *path);
char *ReplaceExt(char *path, char *ext);
char *DirName(char *path);
bool FileExists(char *path);
bool DirExists(char *path);
char *HomeDir(void);
