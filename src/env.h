#pragma once

#include "mem.h"

Val ExtendEnv(Val env, Val frame, Mem *mem);
void Define(Val value, u32 index, Val env, Mem *mem);
Val Lookup(u32 frames, u32 index, Val env, Mem *mem);
i32 FindDefinition(Val value, Val env, Mem *mem);
