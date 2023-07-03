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

typedef enum {
  RegCont = (1 << 0),
  RegEnv  = (1 << 1),
} Reg;
#define NUM_REGS 2

void InitVM(VM *vm);
void RunChunk(VM *vm, Chunk *chunk);

