#pragma once
#include "interpret.h"
#include "mem.h"

Val DoPrimitive(Val op, Val args, Mem *mem);
void DefinePrimitives(Val env, Mem *mem);
