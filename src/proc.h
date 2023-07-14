#pragma once
#include "mem.h"
#include "vm.h"

bool IsPrimitive(Val proc, Mem *mem);
bool IsCompoundProc(Val proc, Mem *mem);
u32 ProcEntry(Val proc, Mem *mem);
Val ProcEnv(Val proc, Mem *mem);
Val ApplyPrimitive(Val proc, u32 num_args, VM *vm);
