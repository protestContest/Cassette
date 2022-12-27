#pragma once
#include "vm.h"

#define IsPrimitiveProc(vm, proc)   IsTaggedList(vm, proc, SymbolFor(vm, "primitive", 9))

Val ApplyPrimitiveProc(VM *vm, Val proc, Val args);
void DefinePrimitives(VM *vm, Val env);
Val MakeProcedure(VM *vm, Val params, Val body, Val env);
