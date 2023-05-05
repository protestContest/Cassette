#pragma once
#include "mem.h"
#include "lex.h"
#include "chunk.h"

typedef enum {
  RegVal,
  RegEnv,
  RegFun,
  RegArg,
  RegCon,

  NUM_REGS
} Reg;

typedef struct {
  Mem *mem;
  Val *stack;
  u32 pc;
  Val regs[NUM_REGS];
  Chunk *chunk;
  Val *modules;
  bool trace;
  struct {
    u32 stack_ops;
    u32 reductions;
  } stats;
} VM;

#define Halt(vm)  ((vm)->pc = VecCount((vm)->chunk->data))

u32 PrintReg(i32 reg);

void InitVM(VM *vm, Mem *mem);
void RunChunk(VM *vm, Chunk *chunk);
Val RuntimeError(char *message, Val exp, VM *vm);
