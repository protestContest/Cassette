#pragma once
#include "mem.h"
#include "lex.h"

typedef struct {
  Mem *mem;
  Val *stack;
  Val env;
  Map ports;
  Buf *buffers;
  void **windows;
  Source src;
} VM;

void InitVM(VM *vm, Mem *mem, Source src);
