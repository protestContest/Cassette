#pragma once
#include "mem.h"
#include "vm.h"

bool IsPrimitive(Val proc, Mem *mem);
Val MakePrimitive(Val num, Mem *mem);
u32 PrimitiveNum(Val primitive, Mem *mem);

bool IsFunction(Val proc, Mem *mem);
Val MakeFunction(Val pos, Val env, Mem *mem);
u32 ProcEntry(Val proc, Mem *mem);
Val ProcEnv(Val proc, Mem *mem);

Val ApplyPrimitive(Val proc, u32 num_args, VM *vm);
