#include "mem.h"
#include "univ/system.h"

Val MakeFunction(Val arity, Val position, Val env, Mem *mem)
{
  Val func;

  if (!CheckMem(mem, 3)) {
    PushRoot(mem, env);
    CollectGarbage(mem);
    env = PopRoot(mem);
  }
  Assert(CheckMem(mem, 3));

  func = ObjVal(MemNext(mem));
  PushMem(mem, FuncHeader(arity));
  PushMem(mem, position);
  PushMem(mem, env);
  return func;
}
