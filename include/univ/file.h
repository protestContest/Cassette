#pragma once

/* Helpers and wrappers for file and filesystem operations */

/* Opens a file */
i32 Open(char *path, i32 flags, char **error);

/* Opens a serial port */
i32 OpenSerial(char *path, i32 speed, i32 opts, char **error);

/* Closes a file */
char *Close(i32 file);

/* Reads up to size bytes from a file into a buffer */
i32 Read(i32 file, char *buf, u32 size, char **error);

/* Writes size bytes from a buffer into a file */
i32 Write(i32 file, char *buf, u32 size, char **error);

/* Seeks to a position in a file */
typedef enum {seek_set, seek_cur, seek_end} SeekMethod;
i32 Seek(i32 file, i32 offset, SeekMethod whence, char **error);

/* Opens a listening socket on a port */
i32 Listen(char *port, char **error);

/* Accepts a connection on a socket */
i32 Accept(i32 sock, char **error);

/* Opens a socket connected to a host */
i32 Connect(char *host, char *port, char **error);

/* Returns the size of a file */
u32 FileSize(char *path);

/* Reads an entire file into memory */
void *ReadFile(char *path);

/* Reads an entire text file into memory */
char *ReadTextFile(char *path);

/* Writes size bytes into a file */
i32 WriteFile(void *data, u32 size, char *path);

typedef struct {
  u32 count;
  char **filenames;
} FileList;

/* Creates a file list */
FileList *NewFileList(u32 count);

/* Frees a file list */
void FreeFileList(FileList *list);

/* Returns a list of files in a given directory. If a list is given, new
 * filenames are added to it. */
FileList *ListFiles(char *path, char *ext, FileList *list);

/* Returns a pointer to the extension of a file path */
char *FileExt(char *path);

/* Returns a copy of path with the extension replaced */
char *ReplaceExt(char *path, char *ext);

/* Returns the part of a path that is the directory */
char *DirName(char *path);

/* Returns whether a file exists at a path */
bool FileExists(char *path);

/* Returns whether a directory exists at a path */
bool DirExists(char *path);

/* Returns the user's home directory */
char *HomeDir(void);
