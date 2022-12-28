#pragma once
#include "vm.h"
#include "list.h"

#define IsPrimitiveProc(vm, proc)   IsTagged(vm, proc, MakeSymbol(vm, "prim", 4))
#define IsCompoundProc(vm, proc)    IsTagged(vm, proc, MakeSymbol(vm, "proc", 4))
#define ProcParams(vm, proc)        TupleAt(vm, proc, 1)
#define ProcBody(vm, proc)          TupleAt(vm, proc, 2)
#define ProcEnv(vm, proc)           TupleAt(vm, proc, 3)

Val ApplyPrimitiveProc(VM *vm, Val proc, Val args);
void DefinePrimitives(VM *vm, Val env);
Val MakeProcedure(VM *vm, Val params, Val body, Val env);
