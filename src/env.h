#pragma once
#include "mem.h"
#include "vm.h"

Val InitialEnv(Mem *mem);
Val TopEnv(Mem *mem, Val env);
Val ExtendEnv(Val env, Mem *mem);
void Define(Val var, Val value, Val env, Mem *mem);
Val Lookup(Val var, Val env, VM *vm);
Val MakeProcedure(Val body, Val env, Mem *mem);
Val ProcBody(Val proc, Mem *mem);
Val ProcEnv(Val proc, Mem *mem);

void PrintEnv(Val env, Mem *mem);
