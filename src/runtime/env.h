#pragma once
#include "mem/mem.h"
#include "mem/symbols.h"

#define Undefined         0x7FDBC789 /* *undefined* */

#define ExtendEnv(env, frame, mem)  Pair(frame, env, mem)
Val InitialEnv(Mem *mem);
void Define(Val value, u32 index, Val env, Mem *mem);
Val Lookup(u32 index, Val env, Mem *mem);
