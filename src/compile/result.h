#pragma once

#include "mem/mem.h"

typedef struct {
  bool ok;
  Val value;
  char *error;
  char *filename;
  char *source;
  u32 pos;
} BuildResult;

BuildResult BuildOk(Val value);
BuildResult BuildError(char *error, char *filename, char *source, u32 pos);
void PrintBuildError(BuildResult error);
