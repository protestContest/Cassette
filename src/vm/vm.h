#pragma once
#include "heap.h"
#include "chunk.h"

typedef enum {
  NoError,
  StackError,
  TypeError,
  ArithmeticError,
  EnvError,
  KeyError,
} VMError;

typedef struct {
  u32 pc;
  u32 cont;
  Val *stack;
  Val *call_stack;
  Val env;
  Heap *mem;
  VMError error;
} VM;

typedef enum {
  RegCont,
  RegEnv,
} Reg;

void InitVM(VM *vm, Heap *mem);
void DestroyVM(VM *vm);
void RunChunk(VM *vm, Chunk *chunk);
void RuntimeError(VM *vm, VMError error);

void StackPush(VM *vm, Val value);
Val StackPop(VM *vm);
