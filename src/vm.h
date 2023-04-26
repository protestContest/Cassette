#pragma once
#include "mem.h"
#include "lex.h"

#define RegVal    Bit(0)
#define RegExp    Bit(1)
#define RegEnv    Bit(2)
#define RegCont   Bit(3)
#define RegProc   Bit(4)
#define RegArgs   Bit(5)
#define RegTmp    Bit(6)
#define RegAll    (RegVal | RegExp | RegEnv | RegCont | RegProc | RegArgs | RegTmp)
typedef i32 Reg;

#define IntToReg(i) (1 << (i))

typedef struct {
  Mem *mem;
  Val *stack;
  Val env;
  Map ports;
  Buf *buffers;
  void **windows;
} VM;

void InitVM(VM *vm, Mem *mem);

void PrintReg(i32 reg);
