#pragma once
#include "mem.h"
#include <univ/window.h>

typedef struct {
  Mem *mem;
  Val *stack;
  Val env;
} VM;

