#pragma once
#include "value.h"
#include "chunk.h"

typedef enum Reg {
  REG_EXP,
  REG_ENV,
  REG_VAL,
  REG_CONT,
  REG_PROC,
  REG_ARGS,
  REG_TMP,
  NUM_REGS
} Reg;

const char *RegStr(Reg reg);

typedef struct {
  Val regs[NUM_REGS];
  bool flag;
  bool trace;
  u32 sp;
  Val *stack;
  Chunk *chunk;
} VM;

VM *NewVM(void);
void PrintRegisters(VM *vm);
void Run(VM *vm, Chunk *chunk);
