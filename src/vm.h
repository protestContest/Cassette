#pragma once
#include "mem.h"
#include "lex.h"
#include "chunk.h"

typedef enum {
  RegVal,
  RegEnv,
  RegFun,
  RegArgs,
  RegCont,
  RegArg1,
  RegArg2,
  RegStack,

  NUM_REGS
} Reg;

typedef struct {
  Mem *mem;
  u32 pc;
  Val regs[NUM_REGS];
  Chunk *chunk;
  bool halted;
  struct {
    u32 stack_ops;
  } stats;
} VM;

u32 PrintReg(i32 reg);

void InitVM(VM *vm, Mem *mem);
void RunChunk(VM *vm, Chunk *chunk);
Val RuntimeError(char *message, Val exp, VM *vm);
