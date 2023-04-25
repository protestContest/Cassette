#include "env.h"
#include "eval.h"
#include "primitives.h"

Val InitialEnv(Mem *mem)
{
  MakeSymbol(mem, "ok");
  MakeSymbol(mem, "error");
  Val env = ExtendEnv(nil, mem);
  Define(MakeSymbol(mem, "nil"), nil, env, mem);
  Define(MakeSymbol(mem, "true"), SymbolFor("true"), env, mem);
  Define(MakeSymbol(mem, "false"), SymbolFor("false"), env, mem);
  DefinePrimitives(env, mem);
  return env;
}

Val TopEnv(Mem *mem, Val env)
{
  while (!IsNil(Tail(mem, env))) {
    env = Tail(mem, env);
  }
  return env;
}

Val ExtendEnv(Val env, Mem *mem)
{
  return MakePair(mem, MakeDict(mem), env);
}

void Define(Val var, Val value, Val env, Mem *mem)
{
  Val frame = Head(mem, env);
  SetHead(mem, env, DictSet(mem, frame, var, value));
}

Val Lookup(Val var, Val env, VM *vm)
{
  while (!IsNil(env)) {
    Val frame = Head(vm->mem, env);
    if (InDict(vm->mem, frame, var)) {
      return DictGet(vm->mem, frame, var);
    }

    env = Tail(vm->mem, env);
  }

  return RuntimeError("Unbound variable", var, vm);
}

Val MakeProcedure(Val params, Val body, Val env, Mem *mem)
{
  return MakeList(mem, 4, MakeSymbol(mem, "λ"), params, body, env);
}

#ifdef DEBUG_EVAL
void PrintEnv(Val env, Mem *mem)
{
  if (IsNil(env)) Print("<empty env>");
  while (!IsNil(env)) {
    Print("----------------\n");
    Val frame = Head(mem, env);

    Val vars = DictKeys(mem, frame);
    Val vals = DictValues(mem, frame);
    for (u32 i = 0; i < DictSize(mem, frame); i++) {
      Val var = TupleAt(mem, vars, i);
      if (IsSym(var)) {
        Print(SymbolName(mem, var));
        Print(": ");
      } else if (!IsNil(var)) {
        PrintVal(mem, var);
        Print(" => ");
      }
      PrintVal(mem, TupleAt(mem, vals, i));
      if (i != TupleLength(mem, frame) - 1) {
        Print(" ");
      }
    }

    env = Tail(mem, env);
  }
}
#endif
