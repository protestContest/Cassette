#include "env.h"
#include "primitives.h"
#include "vm.h"

Val InitialEnv(Mem *mem)
{
  MakeSymbol(mem, "ok");
  MakeSymbol(mem, "error");
  MakeSymbol(mem, "ε");
  Val env = ExtendEnv(nil, mem);
  Define(MakeSymbol(mem, "nil"), nil, env, mem);
  Define(MakeSymbol(mem, "true"), SymbolFor("true"), env, mem);
  Define(MakeSymbol(mem, "false"), SymbolFor("false"), env, mem);
  DefinePrimitives(env, mem);
  return ExtendEnv(env, mem);
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
  return MakePair(mem, MakePair(mem, SymbolFor("ε"), MakeDict(mem)), env);
}

void Define(Val var, Val value, Val env, Mem *mem)
{
  Val frame = Head(mem, env);
  frame = MakePair(mem, SymbolFor("ε"), DictSet(mem, Tail(mem, frame), var, value));
  SetHead(mem, env, frame);
}

Val Lookup(Val var, Val env, VM *vm)
{
  while (!IsNil(env)) {
    Val frame = Tail(vm->mem, Head(vm->mem, env));
    if (InDict(vm->mem, frame, var)) {
      return DictGet(vm->mem, frame, var);
    }

    env = Tail(vm->mem, env);
  }

  RuntimeError("Unbound variable", var, vm);
  return nil;
}

Val MakeProcedure(Val body, Val env, Mem *mem)
{
  return
    MakePair(mem, MakeSymbol(mem, "λ"),
    MakePair(mem, body,
    MakePair(mem, env, nil)));
}

Val ProcBody(Val proc, Mem *mem)
{
  return ListAt(mem, proc, 1);
}

Val ProcEnv(Val proc, Mem *mem)
{
  return ListAt(mem, proc, 2);
}

void PrintEnv(Val env, Mem *mem)
{
  if (IsNil(env)) Print("<empty env>");
  while (!IsNil(env)) {
    Print("----------------\n");
    Val frame = Tail(mem, Head(mem, env));

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
