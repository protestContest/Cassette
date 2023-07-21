#pragma once
#include "mem.h"
#include "vm.h"

bool IsPrimitive(Val func, Mem *mem);
Val MakePrimitive(Val num, Mem *mem);
u32 PrimitiveNum(Val primitive, Mem *mem);

bool IsFunction(Val func, Mem *mem);
Val MakeFunction(Val pos, Val arity, Val env, Mem *mem);
u32 FunctionEntry(Val func, Mem *mem);
u32 FunctionArity(Val func, Mem *mem);
Val FunctionEnv(Val func, Mem *mem);

Val ApplyPrimitive(Val func, u32 num_args, VM *vm);
