#pragma once
#include "mem.h"

Val InitialEnv(Mem *mem);
void Define(Val var, Val value, Val env, Mem *mem);
Val Lookup(Val var, Val env, Mem *mem);
Val LookupPosition(Val var, Val env, Mem *mem);
Val LookupByPosition(Val frame, Val entry, Val env, Mem *mem);

void PrintEnv(Val env, Mem *mem);
