#pragma once
#include "mem.h"
#include "lex.h"
#include "chunk.h"

typedef enum {
  RegVal,
  RegEnv,
  RegCon,
  RegFun,
  RegArg,

  NUM_REGS
} Reg;

typedef struct {
  Mem *mem;
  Val *stack;
  u32 pc;
  Val regs[NUM_REGS];
  bool halted;
  struct {
    u32 stack_ops;
    u32 reductions;
  } stats;
} VM;

u32 PrintReg(i32 reg);

void InitVM(VM *vm, Mem *mem);
void RunChunk(VM *vm, Chunk *chunk);
void RuntimeError(char *message, Val exp, VM *vm);
