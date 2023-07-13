#include "proc.h"
#include "env.h"

typedef Val (*PrimitiveFn)(VM *vm);

static Val PrimRemainder(VM *vm);
static Val PrimPrint(VM *vm);
static Val PrimInspect(VM *vm);

static struct {char *name; PrimitiveFn fn;} primitives[] = {
  {"rem", &PrimRemainder},
  {"print", &PrimPrint},
  {"inspect", &PrimInspect},
};

void DefinePrimitives(Val env, Mem *mem)
{
  for (u32 i = 0; i < ArrayCount(primitives); i++) {
    char *name = primitives[i].name;
    Val var = MakeSymbol(name, mem);
    Val proc =
      Pair(MakeSymbol("@", mem),
      Pair(var,
      Pair(IntVal(i), nil, mem), mem), mem);
    Define(var, proc, env, mem);
  }
}

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
  return primitives[index].fn(vm);
}

static Val PrimRemainder(VM *vm)
{
  Val a = StackPop(vm);
  Val b = StackPop(vm);
  if (!IsInt(a) || !IsInt(b)) {
    RuntimeError(vm, "Bad rem argument");
    return nil;
  }

  i32 div = RawInt(a) / RawInt(b);
  i32 result = RawInt(a) - RawInt(b)*div;
  return IntVal(result);
}

static Val PrimPrint(VM *vm)
{
  Val val = StackPop(vm);
  if (PrintVal(val, &vm->mem)) {
    Print("\n");
    return SymbolFor("ok");
  } else {
    return SymbolFor("error");
  }
}

static Val PrimInspect(VM *vm)
{
  Val val = StackPop(vm);
  InspectVal(val, &vm->mem);
  Print("\n");
  return val;
}
