#include "proc.h"
#include "mem.h"

Val MakeProc(Val name, Val params, Val body, Val env)
{
  return MakeTuple(5, MakeSymbol("proc", 4), name, params, body, env);
}

Val ProcName(Val proc)
{
  return TupleAt(proc, 1);
}

Val ProcParams(Val proc)
{
  return TupleAt(proc, 2);
}

Val ProcBody(Val proc)
{
  return TupleAt(proc, 3);
}

Val ProcEnv(Val proc)
{
  return TupleAt(proc, 4);
}
