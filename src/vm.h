#pragma once
#include "value.h"
#include "chunk.h"

typedef enum RunResult {
  ResultOk,
  ResultCompileError,
  ResultRuntimeError,
} Result;

typedef struct VM {
  u32 pc;
  Chunk *chunk;
  Val *stack;
  char *error;
} VM;

VM *NewVM(void);
void FreeVM(VM *vm);
Result Run(VM *vm, Chunk *chunk);
Result RuntimeError(VM *vm, char *msg);
