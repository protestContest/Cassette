#pragma once
#include "value.h"
#include "chunk.h"

typedef struct VM {
  Status status;
  u32 pc;
  Chunk *chunk;
  Val *stack;
} VM;

VM *NewVM(void);
void FreeVM(VM *vm);
Status Interpret(VM *vm, char *src);
void RuntimeError(VM *vm, char *fmt, ...);
