#include "proc.h"
#include "primitives.h"

bool IsPrimitive(Val proc, Mem *mem)
{
  return IsPair(proc) && Eq(Head(proc, mem), SymbolFor("@"));
}

bool IsCompoundProc(Val proc, Mem *mem)
{
  return IsPair(proc) && Eq(Head(proc, mem), SymbolFor("λ"));
}

u32 ProcEntry(Val proc, Mem *mem)
{
  return RawInt(ListAt(proc, 1, mem));
}

Val ProcEnv(Val proc, Mem *mem)
{
  return ListAt(proc, 2, mem);
}

Val ApplyPrimitive(Val proc, VM *vm)
{
  u32 index = RawInt(ListAt(proc, 2, &vm->mem));
  return GetPrimitive(index)(vm);
}
