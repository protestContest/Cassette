#pragma once
#include "result.h"
#include "runtime/vm.h"

typedef struct {
  bool debug;
  u32 num_files;
  char **filenames;
  char *stdlib_path;
} Options;

int Usage(void);
Options ParseOpts(u32 argc, char *argv[]);
void PrintRuntimeError(Result error, VM *vm);
void PrintStackTrace(Val stack, VM *vm);
void PrintError(Result error);
