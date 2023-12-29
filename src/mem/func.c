#include "mem.h"
#include "univ/system.h"
#include <stdio.h>

Val MakeFunction(Val arity, Val position, Val env, Mem *mem)
{
  Val func;
  Assert(CheckMem(mem, 3));
  func = ObjVal(MemNext(mem));
  PushMem(mem, FuncHeader(arity));
  PushMem(mem, position);
  PushMem(mem, env);
  return func;
}
