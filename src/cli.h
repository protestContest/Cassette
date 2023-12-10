#pragma once
#include "result.h"

typedef struct {
  bool trace;
  u32 file_args;
  char *stdlib_path;
} Options;

int Usage(void);
Options ParseOpts(u32 argc, char *argv[]);
void PrintError(Result error);
