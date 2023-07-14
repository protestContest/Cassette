#include "primitives.h"
#include "env.h"

static Val PrimRemainder(VM *vm);
static Val PrimPrint(VM *vm);
static Val PrimInspect(VM *vm);
static Val PrimRandom(VM *vm);
static Val PrimCeil(VM *vm);
static Val PrimFloor(VM *vm);
static Val PrimAbs(VM *vm);
static Val PrimSin(VM *vm);
static Val PrimCos(VM *vm);
static Val PrimTan(VM *vm);
static Val PrimLog(VM *vm);
static Val PrimExp(VM *vm);

static struct {char *mod; char *name; PrimitiveFn fn;} primitives[] = {
  {NULL, "print", &PrimPrint},
  {NULL, "inspect", &PrimInspect},
  {"Math", "rem", &PrimRemainder},
  {"Math", "random", &PrimRandom},
  {"Math", "ceil", &PrimCeil},
  {"Math", "floor", &PrimFloor},
  {"Math", "abs", &PrimAbs},
  // {"Math", "sin", &PrimSin},
  // {"Math", "cos", &PrimCos},
  // {"Math", "tan", &PrimTan},
  // {"Math", "log", &PrimLog},
  // {"Math", "exp", &PrimExp},
};

PrimitiveFn GetPrimitive(u32 index)
{
  return primitives[index].fn;
}

void DefinePrimitives(Val env, Mem *mem)
{
  Map modules;
  InitMap(&modules);

  for (u32 i = 0; i < ArrayCount(primitives); i++) {
    char *name = primitives[i].name;
    Val var = MakeSymbol(name, mem);
    Val proc =
      Pair(MakeSymbol("@", mem),
      Pair(var,
      Pair(IntVal(i), nil, mem), mem), mem);

    if (primitives[i].mod == NULL) {
      Define(var, proc, env, mem);
    } else {
      Val name = MakeSymbol(primitives[i].mod, mem);
      Val mod;
      if (MapContains(&modules, name.as_i)) {
        mod = (Val){.as_i = MapGet(&modules, name.as_i)};
      } else {
        mod = MakeValMap(mem);
        MapSet(&modules, name.as_i, mod.as_i);
        Define(name, mod, env, mem);
      }
      ValMapSet(mod, var, proc, mem);
    }
  }

  DestroyMap(&modules);
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

static Val PrimRandom(VM *vm)
{
  return IntVal(Random());
}

static Val PrimCeil(VM *vm)
{
  Val n = StackPop(vm);
  if (IsInt(n)) return n;

  if (IsNum(n)) {
    return IntVal((i32)(RawNum(n) - 0.5) + 1);
  }

  RuntimeError(vm, "Bad arg to ceil");
  return nil;
}

static Val PrimFloor(VM *vm)
{
  Val n = StackPop(vm);
  if (IsInt(n)) return n;

  if (IsNum(n)) {
    return IntVal((i32)(RawNum(n) - 0.5));
  }

  RuntimeError(vm, "Bad arg to floor");
  return nil;
}

static Val PrimAbs(VM *vm)
{
  Val n = StackPop(vm);
  if (!IsNumeric(n)) {
    RuntimeError(vm, "Bad arg to abs");
    return nil;
  }

  if (RawNum(n) >= 0) return n;

  if (IsInt(n)) return IntVal(-RawInt(n));
  return NumVal(-RawNum(n));
}

// static Val PrimSin(VM *vm)
// {

// }

// static Val PrimCos(VM *vm)
// {

// }

// static Val PrimTan(VM *vm)
// {

// }

// static Val PrimLog(VM *vm)
// {

// }

// static Val PrimExp(VM *vm)
// {

// }
