#include "primitives.h"
#include "env.h"
#include "proc.h"

static Val PrimRemainder(u32 num_args, VM *vm);
static Val PrimPrint(u32 num_args, VM *vm);
static Val PrimInspect(u32 num_args, VM *vm);
static Val PrimRandom(u32 num_args, VM *vm);
static Val PrimCeil(u32 num_args, VM *vm);
static Val PrimFloor(u32 num_args, VM *vm);
static Val PrimAbs(u32 num_args, VM *vm);
static Val PrimSin(u32 num_args, VM *vm);
static Val PrimCos(u32 num_args, VM *vm);
static Val PrimTan(u32 num_args, VM *vm);
static Val PrimLog(u32 num_args, VM *vm);
static Val PrimExp(u32 num_args, VM *vm);

static struct {char *mod; char *name; PrimitiveFn fn;} primitives[] = {
  {NULL, "print", &PrimPrint},
  {NULL, "inspect", &PrimInspect},
  {NULL, "rem", &PrimRemainder},
  {NULL, "random", &PrimRandom},
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
    Val proc = MakePrimitive(IntVal(i), mem);

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

static Val PrimRemainder(u32 num_args, VM *vm)
{
  if (num_args != 2) {
    RuntimeError(vm, "Wrong number of arguments to rem");
    return nil;
  }

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

static Val PrimPrint(u32 num_args, VM *vm)
{
  for (u32 i = 0; i < num_args; i++) {
    Val val = StackPop(vm);
    if (!PrintVal(val, &vm->mem)) {
      return MakeSymbol("error", &vm->mem);
    }

    if (i < num_args-1) Print(" ");
  }
  Print("\n");
  return MakeSymbol("ok", &vm->mem);
}

static Val PrimInspect(u32 num_args, VM *vm)
{
  for (u32 i = 0; i < num_args; i++) {
    Val val = StackPop(vm);
    InspectVal(val, &vm->mem);
    Print("\n");
  }
  return MakeSymbol("ok", &vm->mem);
}

static Val PrimRandom(u32 num_args, VM *vm)
{
  u32 r = Random();

  if (num_args > 0) {
    Val max = StackPop(vm);
    if (!IsInt(max)) {
      RuntimeError(vm, "Bad argument to random");
      return nil;
    }

    Val min = IntVal(0);
    if (num_args > 1) {
      min = max;
      max = StackPop(vm);
      if (!IsInt(min)) {
        RuntimeError(vm, "Bad argument to random");
        return nil;
      }
    }

    if (num_args > 2) {
      RuntimeError(vm, "Wrong number of arguments to random");
      return nil;
    }

    u32 range = RawInt(max) - RawInt(min);
    float scaled = r * (float)range / MaxUInt;
    u32 biased = (u32)(scaled + RawInt(min));
    return IntVal(biased);
  }

  return NumVal((float)r / MaxUInt);
}

static Val PrimCeil(u32 num_args, VM *vm)
{
  if (num_args != 1) {
    RuntimeError(vm, "Wrong number of arguments to ceil");
    return nil;
  }

  Val n = StackPop(vm);
  if (IsInt(n)) return n;

  if (IsNum(n)) {
    return IntVal((i32)(RawNum(n) - 0.5) + 1);
  }

  RuntimeError(vm, "Bad arg to ceil");
  return nil;
}

static Val PrimFloor(u32 num_args, VM *vm)
{
  if (num_args != 1) {
    RuntimeError(vm, "Wrong number of arguments to floor");
    return nil;
  }

  Val n = StackPop(vm);
  if (IsInt(n)) return n;

  if (IsNum(n)) {
    return IntVal((i32)(RawNum(n) - 0.5));
  }

  RuntimeError(vm, "Bad arg to floor");
  return nil;
}

static Val PrimAbs(u32 num_args, VM *vm)
{
  if (num_args != 1) {
    RuntimeError(vm, "Wrong number of arguments to abs");
    return nil;
  }

  Val n = StackPop(vm);
  if (!IsNumeric(n)) {
    RuntimeError(vm, "Bad arg to abs");
    return nil;
  }

  if (RawNum(n) >= 0) return n;

  if (IsInt(n)) return IntVal(-RawInt(n));
  return NumVal(-RawNum(n));
}

// static Val PrimSin(u32 num_args, VM *vm)
// {

// }

// static Val PrimCos(u32 num_args, VM *vm)
// {

// }

// static Val PrimTan(u32 num_args, VM *vm)
// {

// }

// static Val PrimLog(u32 num_args, VM *vm)
// {

// }

// static Val PrimExp(u32 num_args, VM *vm)
// {

// }
