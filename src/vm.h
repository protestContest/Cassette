#pragma once
#include "value.h"
#include "chunk.h"

typedef struct VM {
  u32 pc;
  Chunk *chunk;
  Val *stack;
  char *error;
} VM;

VM *NewVM(void);
void FreeVM(VM *vm);
Status Interpret(VM *vm, char *src);
Status RuntimeError(VM *vm, char *msg);
