#pragma once

typedef struct {
  char *message;
  u32 pos;
  char *filename;
} ErrorDetail;

typedef struct {
  bool ok;
  void *value;
} Result;

#define IsOk(result)  ((result).ok)

Result Ok(void *value);
Result Error(char *message, char *filename, u32 pos);
void FreeResult(Result result);
void PrintError(Result result);
