#pragma once

#include "mem/mem.h"

typedef struct {
  bool ok;
  char *error;
  char *filename;
  u32 pos;
  Val value;
  void *data;
} Result;

Result OkResult(Val value);
Result DataResult(void *data);
Result ErrorResult(char *error, char *filename, u32 pos);
Result OkError(Result error, Mem *mem);
