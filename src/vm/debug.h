#pragma once
#include "vm.h"

void TraceHeader(void);
void TraceInstruction(VM *vm, Chunk *chunk);
void PrintEnv(Val env, Heap *mem);
