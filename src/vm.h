#pragma once
#include "mem.h"
#include "lex.h"
#include "chunk.h"

// #define DEBUG_VM

#define RegVal    Bit(0)
#define RegExp    Bit(1)
#define RegEnv    Bit(2)
#define RegCont   Bit(3)
#define RegProc   Bit(4)
#define RegArgs   Bit(5)
#define RegAll    (RegVal | RegExp | RegEnv | RegCont | RegProc | RegArgs)
#define NUM_REGS  6
typedef i32 Reg;

#define IntToReg(i) (1 << (i))

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
