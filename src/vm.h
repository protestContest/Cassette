#pragma once
#include "chunk.h"
#include "mem.h"

typedef struct {
  u32 pc;
  Val cont;
  Val env;
  Val *val;
  Val *stack;
  Mem mem;
  Map modules;
  Chunk *chunk;
  bool error;
  bool trace;
  u32 gc_threshold;
} VM;

typedef enum {
  RegCont = (1 << 0),
  RegEnv  = (1 << 1),
} Reg;
#define NUM_REGS 2

void InitVM(VM *vm);
void DestroyVM(VM *vm);
Val RunChunk(VM *vm, Chunk *chunk);
void RuntimeError(VM *vm, char *message);

Val StackPop(VM *vm);
Val StackPeek(VM *vm, u32 n);
void StackPush(VM *vm, Val val);

void TakeOutGarbage(VM *vm);

void PrintStack(VM *vm);
void PrintCallStack(VM *vm);
