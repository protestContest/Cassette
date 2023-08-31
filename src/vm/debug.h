#pragma once
#include "vm.h"

void TraceInstruction(VM *vm, Chunk *chunk);
void PrintEnv(Val env, Heap *mem);
