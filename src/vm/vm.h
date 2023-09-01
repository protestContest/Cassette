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
  ArgError,
} VMError;

typedef struct {
  u32 pc;
  u32 cont;
  Val *stack;
  Val *call_stack;
  Val env;
  Val *modules;
  HashMap mod_map;
  Heap *mem;
  VMError error;
  CassetteOpts *opts;
} VM;

typedef enum {
  RegCont,
  RegEnv,
} Reg;

void InitVM(VM *vm, CassetteOpts *opts, Heap *mem);
void DestroyVM(VM *vm);
void RunChunk(VM *vm, Chunk *chunk);

void StackPush(VM *vm, Val value);
Val StackPop(VM *vm);

#ifndef LIBCASSETTE
void PrintOpStats(void);
#endif
