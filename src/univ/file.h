#pragma once
#include "vec.h"
#include <unistd.h>
#include <sys/fcntl.h>

/* 0644 */
#define FileMode                  S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR

#define Read(file, buf, size)     read(file, buf, size)
#define Write(file, buf, size)    write(file, buf, size)
#define Open(path)                open(path, O_RDWR, 0)
#define Close(file)               close(file)
#define Truncate(file)            ftruncate(file, 0)
#define MakeFile(path)            open(path, O_RDWR | O_CREAT, FileMode)

u32 FileSize(char *filename);
char *ReadFile(char *path);
char *FileExt(char *name);
bool DirExists(char *path);
void DirContents(char *path, char *ext, ObjVec *contents);
