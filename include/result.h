#pragma once

/* A general-purpose result structure */

typedef struct {
  bool ok;
  union {
    void *p;
    u32 v;
  } data;
} Result;
#define IsError(result)    (!(result).ok)

typedef struct {
  char *message;
  char *file;
  u32 pos;
  u32 length;
} Error;

Result Ok(void *data);
Result OkVal(u32 value);
Result Err(Error *error);
Error *NewError(char *message, char *file, u32 pos, u32 length);
void PrintError(Result result);
