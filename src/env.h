#pragma once
#include "mem.h"

Val InitialEnv(Mem *mem);
Val TopEnv(Mem *mem, Val env);
Val ExtendEnv(Val env, Mem *mem);
void Define(Val var, Val value, Val env, Mem *mem);
Val Lookup(Val exp, Val env, Mem *mem);
void PrintEnv(Val env, Mem *mem);
