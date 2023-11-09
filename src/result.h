#pragma once

#include "mem/mem.h"

typedef struct {
  bool ok;
  char *error;
  char *filename;
  u32 pos;
  Val value;
} Result;

Result OkResult(Val value);
Result ErrorResult(char *error, char *filename, u32 pos);
