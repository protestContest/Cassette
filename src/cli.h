#pragma once
#include "result.h"
#include "runtime/vm.h"
#include "runtime/stacktrace.h"

typedef struct {
  bool debug;
  bool compile;
  u32 num_files;
  char **filenames;
  char *stdlib_path;
} Options;

int Usage(void);
Options ParseOpts(u32 argc, char *argv[]);
void PrintRuntimeError(Result error, VM *vm);
void PrintStackTrace(StackTraceItem *stack, VM *vm);
void PrintError(Result error);
void WriteChunk(Chunk *chunk, char *filename);
