#pragma once
#include "result.h"

typedef struct {
  bool project;
  bool step;
  u32 file_args;
} Options;

int Usage(void);
Options ParseOpts(u32 argc, char *argv[]);
bool WaitForInput(void);
void PrintError(Result error);
