#pragma once
#include "mem.h"

Val InitialEnv(Mem *mem);
Val ExtendEnv(Val env, Mem *mem);
void Define(Val var, Val val, Val env, Mem *mem);
Val Lookup(Val var, Val env, Mem *mem);
void ImportEnv(Val imports, Val env, Mem *mem);
Val ExportEnv(Val env, Mem *mem);

void PrintEnv(Val env, Mem *mem);
