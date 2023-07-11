#pragma once
#include "mem.h"

Val InitialEnv(Mem *mem);
Val ExtendEnv(Val env, Mem *mem);
void Define(Val var, Val val, Val env, Mem *mem);
Val Lookup(Val var, Val env, Mem *mem);
