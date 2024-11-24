#pragma once
#include "runtime/source_map.h"

typedef struct StackTrace {
  char *filename;
  u32 pos;
  struct StackTrace *next;
} StackTrace;

typedef struct {
  char *message;
  char *filename;
  u32 pos;
  u32 length;
  StackTrace *stacktrace;
} Error;

Error *NewError(char *message, char *filename, u32 pos, u32 length);
Error *NewRuntimeError(char *message, u32 pos, u32 link, SourceMap *srcmap);
void FreeError(Error *error);
void PrintError(Error *error);
void PrintSourceLine(char *text, u32 pos);

StackTrace *NewStackTrace(StackTrace *next);
void FreeStackTrace(StackTrace *st);
StackTrace *BuildStackTrace(u32 link, SourceMap *srcmap);
void PrintStackTrace(StackTrace *st);