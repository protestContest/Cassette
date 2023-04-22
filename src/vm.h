#pragma once
#include "mem.h"

typedef struct {
  Mem *mem;
  Val *stack;
  Val env;
  Map ports;
  Buf *buffers;
  void **windows;
} VM;

void InitVM(VM *vm, Mem *mem);
