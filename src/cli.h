#pragma once
#include "result.h"
#include "runtime/vm.h"

typedef struct {
  bool trace;
  u32 file_args;
  char *stdlib_path;
} Options;

int Usage(void);
Options ParseOpts(u32 argc, char *argv[]);
void PrintRuntimeError(Result error, VM *vm);
void PrintStackTrace(Val stack, VM *vm);
void PrintError(Result error);
