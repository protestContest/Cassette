#pragma once
#include "heap.h"

Val InitialEnv(Heap *mem);
Val CompileEnv(Heap *mem);
Val ExtendEnv(Val env, Val frame, Heap *mem);
Val RestoreEnv(Val env, Heap *mem);
void Define(u32 pos, Val item, Val env, Heap *mem);
Val Lookup(Val env, u32 frame, u32 pos, Heap *mem);
Val FindVar(Val var, Val env, Heap *mem);

