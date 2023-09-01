#pragma once
#include "vm.h"

#ifndef LIBCASSETTE
void TraceHeader(void);
void TraceInstruction(VM *vm, Chunk *chunk);
void PrintEnv(Val env, Heap *mem);
#endif
