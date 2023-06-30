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
  Chunk *chunk;
} VM;

#define RegCont 1
#define RegEnv  2

void InitVM(VM *vm);
void RunChunk(VM *vm, Chunk *chunk);

