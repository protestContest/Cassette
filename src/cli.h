#pragma once
#include "runtime/stacktrace.h"
#include "runtime/vm.h"
#include "univ/result.h"

typedef struct {
  bool debug;
  bool compile;
  u32 num_files;
  char **filenames;
  char *stdlib_path;
} Options;

Options ParseOpts(u32 argc, char *argv[]);
void PrintError(Result error);
void PrintRuntimeError(Result error, VM *vm);
void PrintStackTrace(StackTraceItem *stack, VM *vm);
void WriteChunk(Chunk *chunk, char *filename);
Result ReadChunk(char *filename);
