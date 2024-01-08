#pragma once
#include "vec.h"

i32 Read(i32 file, void *buf, u32 size);
i32 Write(i32 file, void *buf, u32 size);
int Open(char *path);
void Close(int file);
void Truncate(int file);
int CreateOrOpen(char *path);
u32 FileSize(char *filename);
char *ReadFile(char *path);
char *FileExt(char *name);
bool DirExists(char *path);
void DirContents(char *path, char *ext, ObjVec *contents);
void WriteInt(u32 n, u8 *data);
u32 ReadInt(u8 *data);
void WriteShort(u16 n, u8 *data);
u16 ReadShort(u8 *data);
