#pragma once

#include "mem/mem.h"
#include "mem/symbols.h"

#define ExtendEnv(env, frame, mem)  Pair(frame, env, mem)
Val InitialEnv(Mem *mem);
void Define(Val value, u32 index, Val env, Mem *mem);
Val Lookup(u32 index, Val env, Mem *mem);
i32 FindDefinition(Val value, Val env, Mem *mem);
i32 FindModule(Val mod, Val env, Mem *mem);
