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
static Val PrimSqrt(u32 num_args, VM *vm);
static Val PrimRound(u32 num_args, VM *vm);
static Val PrimListFrom(u32 num_args, VM *vm);
static Val PrimListTake(u32 num_args, VM *vm);
static Val PrimListSlice(u32 num_args, VM *vm);
static Val PrimListConcat(u32 num_args, VM *vm);
static Val PrimListAppend(u32 num_args, VM *vm);
static Val PrimListFlatten(u32 num_args, VM *vm);
static Val PrimListToBinary(u32 num_args, VM *vm);
static Val PrimListToTuple(u32 num_args, VM *vm);
static Val PrimListToMap(u32 num_args, VM *vm);
static Val PrimListInsert(u32 num_args, VM *vm);
static Val PrimMapGet(u32 num_args, VM *vm);
static Val PrimMapPut(u32 num_args, VM *vm);
static Val PrimMapDelete(u32 num_args, VM *vm);
static Val PrimMapKeys(u32 num_args, VM *vm);
static Val PrimMapValues(u32 num_args, VM *vm);
static Val PrimMapToList(u32 num_args, VM *vm);

static struct {char *mod; char *name; PrimitiveFn fn;} primitives[] = {
  {NULL, "print", &PrimPrint},
  {NULL, "inspect", &PrimInspect},
  {NULL, "rem", &PrimRemainder},
  {NULL, "random", &PrimRandom},
  {"Math", "ceil", &PrimCeil},
  {"Math", "floor", &PrimFloor},
  {"Math", "round", &PrimRound},
  {"Math", "abs", &PrimAbs},
  {"Math", "sqrt", &PrimSqrt},
  {"Math", "log", &PrimLog},
  {"Math", "exp", &PrimExp},
  {"Math", "sin", &PrimSin},
  {"Math", "cos", &PrimCos},
  {"Math", "tan", &PrimTan},
  {"List", "from", &PrimListFrom},
  {"List", "take", &PrimListTake},
  {"List", "slice", &PrimListSlice},
  {"List", "concat", &PrimListConcat},
  {"List", "append", &PrimListAppend},
  {"List", "flatten", &PrimListFlatten},
  {"List", "to_binary", &PrimListToBinary},
  {"List", "to_tuple", &PrimListToTuple},
  {"List", "to_map", &PrimListToMap},
  {"List", "insert", &PrimListInsert},
  {"Map", "get", &PrimMapGet},
  {"Map", "put", &PrimMapPut},
  {"Map", "delete", &PrimMapDelete},
  {"Map", "keys", &PrimMapKeys},
  {"Map", "values", &PrimMapValues},
  {"Map", "to_list", &PrimMapToList},
};

static struct {char *mod; char *name; Val val;} constants[] = {
  {"Math", "Pi", NumVal(Pi)},
  {"Math", "E", NumVal(E)},
};

PrimitiveFn GetPrimitive(u32 index)
{
  return primitives[index].fn;
}

void DefinePrimitives(Val env, Mem *mem)
{
  Map modules;
  InitMap(&modules);

  // get a count of primitives + constants for each module
  for (u32 i = 0; i < ArrayCount(primitives); i++) {
    if (primitives[i].mod == NULL) continue;
    Val mod_name = MakeSymbol(primitives[i].mod, mem);
    u32 count = 0;
    if (MapContains(&modules, mod_name.as_i)) {
      count = MapGet(&modules, mod_name.as_i);
    }
    MapSet(&modules, mod_name.as_i, count + 1);
  }
  for (u32 i = 0; i < ArrayCount(constants); i++) {
    if (constants[i].mod == NULL) continue;
    Val mod_name = MakeSymbol(constants[i].mod, mem);
    u32 count = 0;
    if (MapContains(&modules, mod_name.as_i)) {
      count = MapGet(&modules, mod_name.as_i);
    }
    MapSet(&modules, mod_name.as_i, count + 1);
  }

  // create each module
  for (u32 i = 0; i < MapCount(&modules); i++) {
    Val mod_name = (Val){.as_i = GetMapKey(&modules, i)};
    u32 count = MapGet(&modules, mod_name.as_i);
    Val module = MakeValMap(count, mem);
    Define(mod_name, module, env, mem);
    MapSet(&modules, mod_name.as_i, module.as_i);
  }

  // define primitives for each module
  for (u32 i = 0; i < ArrayCount(primitives); i++) {
    Val primitive_name = MakeSymbol(primitives[i].name, mem);
    Val func = MakePrimitive(IntVal(i), mem);

    if (primitives[i].mod == NULL) {
      Define(primitive_name, func, env, mem);
    } else {
      Val mod_name = SymbolFor(primitives[i].mod);
      Val module = (Val){.as_i = MapGet(&modules, mod_name.as_i)};
      ValMapSet(module, primitive_name, func, mem);
    }
  }

  // define constants for each module
  for (u32 i = 0; i < ArrayCount(constants); i++) {
    Val const_name = MakeSymbol(constants[i].name, mem);
    Val value = constants[i].val;

    if (constants[i].mod == NULL) {
      Define(const_name, value, env, mem);
    } else {
      Val mod_name = SymbolFor(constants[i].mod);
      Val module = (Val){.as_i = MapGet(&modules, mod_name.as_i)};
      ValMapSet(module, const_name, value, mem);
    }
  }

  DestroyMap(&modules);
}

static bool CheckArg(Val arg, ValType type, Mem *mem)
{
  switch (type) {
  case ValAny:      return true;
  case ValNum:      return IsNumeric(arg);
  case ValInt:      return IsInt(arg);
  case ValSym:      return IsSym(arg);
  case ValPair:     return IsPair(arg);
  case ValTuple:    return IsTuple(arg, mem);
  case ValBinary:   return IsBinary(arg, mem);
  case ValMap:      return IsValMap(arg, mem);
  }
}

static bool CheckArgs(ValType *types, u32 num_expected, u32 num_args, VM *vm)
{
  if (num_expected != num_args) {
    RuntimeError(vm, "Wrong number of arguments");
    return false;
  }

  for (u32 i = 0; i < num_args; i++) {
    Val arg = StackPeek(vm, i);

    if (!CheckArg(arg, types[i], &vm->mem)) {
      RuntimeError(vm, "Bad argument");
      return false;
    }
  }

  return true;
}

static Val PrimRemainder(u32 num_args, VM *vm)
{
  ValType types[] = {ValInt, ValInt};
  if (!CheckArgs(types, 2, num_args, vm)) return nil;

  Val a = StackPop(vm);
  Val b = StackPop(vm);

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

  if (num_args == 0) {
    return NumVal((float)r / MaxUInt);
  }

  if (num_args == 1) {
    ValType types[] = {ValInt};
    if (!CheckArgs(types, 1, 1, vm)) return nil;

    Val max = StackPop(vm);
    float scaled = r * (float)RawInt(max) / MaxUInt;
    return IntVal(scaled);
  }

  ValType types[] = {ValInt, ValInt};
  if (!CheckArgs(types, 2, num_args, vm)) return nil;
  Val min = StackPop(vm);
  Val max = StackPop(vm);
  u32 range = RawInt(max) - RawInt(min);
  float scaled = r * (float)range / MaxUInt;
  u32 biased = (u32)(scaled + RawInt(min));
  return IntVal(biased);
}

static Val PrimCeil(u32 num_args, VM *vm)
{
  ValType types[] = {ValNum};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Val n = StackPop(vm);
  return IntVal(Ceil(RawNum(n)));
}

static Val PrimFloor(u32 num_args, VM *vm)
{
  ValType types[] = {ValNum};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Val n = StackPop(vm);
  return IntVal(Floor(RawNum(n)));
}

static Val PrimRound(u32 num_args, VM *vm)
{
  ValType types[] = {ValNum};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;
  Val n = StackPop(vm);
  return IntVal(Round(RawNum(n)));
}

static Val PrimAbs(u32 num_args, VM *vm)
{
  ValType types[] = {ValNum};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Val n = StackPop(vm);
  return IntVal(Abs(RawNum(n)));
}

static Val PrimSin(u32 num_args, VM *vm)
{
  ValType types[] = {ValNum};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;
  Val n = StackPop(vm);
  return NumVal(Sin(RawNum(n)));
}

static Val PrimCos(u32 num_args, VM *vm)
{
  ValType types[] = {ValNum};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;
  Val n = StackPop(vm);
  return NumVal(Cos(RawNum(n)));
}

static Val PrimTan(u32 num_args, VM *vm)
{
  ValType types[] = {ValNum};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;
  Val n = StackPop(vm);
  return NumVal(Tan(RawNum(n)));
}

static Val PrimLog(u32 num_args, VM *vm)
{
  ValType types[] = {ValNum};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;
  Val n = StackPop(vm);
  return NumVal(Log(RawNum(n)));
}

static Val PrimExp(u32 num_args, VM *vm)
{
  ValType types[] = {ValNum};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;
  Val n = StackPop(vm);
  return NumVal(Exp(RawNum(n)));
}

static Val PrimSqrt(u32 num_args, VM *vm)
{
  ValType types[] = {ValNum};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;
  Val n = StackPop(vm);
  return NumVal(Sqrt(RawNum(n)));
}

static Val PrimListFrom(u32 num_args, VM *vm)
{
  ValType types[] = {ValPair, ValInt};
  if (!CheckArgs(types, 2, num_args, vm)) return nil;

  Val list = StackPop(vm);
  Val n = StackPop(vm);

  return ListFrom(list, RawInt(n), &vm->mem);
}

static Val PrimListTake(u32 num_args, VM *vm)
{
  ValType types[] = {ValPair, ValInt};
  if (!CheckArgs(types, 2, num_args, vm)) return nil;

  Val list = StackPop(vm);
  Val n = StackPop(vm);

  return ListTake(list, RawInt(n), &vm->mem);
}

static Val PrimListSlice(u32 num_args, VM *vm)
{
  ValType types[] = {ValPair, ValInt, ValInt};
  if (!CheckArgs(types, 3, num_args, vm)) return nil;

  Val list = StackPop(vm);
  Val start = StackPop(vm);
  Val end = StackPop(vm);

  return ListTake(ListFrom(list, RawInt(start), &vm->mem), RawInt(end), &vm->mem);
}

static Val PrimListConcat(u32 num_args, VM *vm)
{
  ValType types[] = {ValPair, ValPair};
  if (!CheckArgs(types, 2, num_args, vm)) return nil;

  Val a = StackPop(vm);
  Val b = StackPop(vm);
  Val list = ListTake(a, ListLength(a, &vm->mem), &vm->mem);
  return ListConcat(list, b, &vm->mem);
}

static Val PrimListAppend(u32 num_args, VM *vm)
{
  ValType types[] = {ValPair, ValAny};
  if (!CheckArgs(types, 2, num_args, vm)) return nil;

  Val list = StackPop(vm);
  Val item = StackPop(vm);
  list = ListTake(list, ListLength(list, &vm->mem), &vm->mem);
  return ListConcat(list, Pair(item, nil, &vm->mem), &vm->mem);
}

static Val PrimListFlatten(u32 num_args, VM *vm)
{
  ValType types[] = {ValPair};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Val list = StackPop(vm);
  return ListFlatten(list, &vm->mem);
}

static bool IOLength(Val list, u32 *length, Mem *mem)
{
  if (IsNil(list)) return true;

  Val item = Head(list, mem);
  if (IsBinary(item, mem)) {
    *length += BinaryLength(item, mem);
  } else if (IsInt(item)) {
    *length += 1;
  } else {
    return false;
  }

  return IOLength(Tail(list, mem), length, mem);
}

static Val PrimListToBinary(u32 num_args, VM *vm)
{
  ValType types[] = {ValPair};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Val list = ListFlatten(StackPop(vm), &vm->mem);
  u32 length = 0;
  if (!IOLength(list, &length, &vm->mem)) {
    RuntimeError(vm, "Could not convert list to binary");
    return nil;
  }

  Val bin = MakeBinary(length, &vm->mem);
  char *data = BinaryData(bin, &vm->mem);
  u32 i = 0;
  while (!IsNil(list)) {
    Val item = Head(list, &vm->mem);
    if (IsInt(item)) {
      data[i] = RawInt(item);
      i++;
    } else if (IsBinary(item, &vm->mem)) {
      char *item_data = BinaryData(item, &vm->mem);
      Copy(item_data, data + i, BinaryLength(item, &vm->mem));
      i += BinaryLength(item, &vm->mem);
    }
    list = Tail(list, &vm->mem);
  }

  return bin;
}

static Val PrimListToTuple(u32 num_args, VM *vm)
{
  ValType types[] = {ValPair};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Val list = StackPop(vm);
  Val tuple = MakeTuple(ListLength(list, &vm->mem), &vm->mem);

  u32 i = 0;
  while (!IsNil(list)) {
    TupleSet(tuple, i, Head(list, &vm->mem), &vm->mem);
    i++;
    list = Tail(list, &vm->mem);
  }

  return tuple;
}

static Val PrimListToMap(u32 num_args, VM *vm)
{
  ValType types[] = {ValPair};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Val list = StackPop(vm);
  u32 count = ListLength(list, &vm->mem);
  Val map = MakeValMap(count, &vm->mem);
  while (!IsNil(list)) {
    Val item = Head(list, &vm->mem);

    if (!IsPair(item)) {
      RuntimeError(vm, "Could not convert list to map");
      return nil;
    }

    ValMapSet(map, Head(item, &vm->mem), Tail(item, &vm->mem), &vm->mem);

    list = Tail(list, &vm->mem);
  }

  return map;
}

static Val PrimListInsert(u32 num_args, VM *vm)
{
  ValType types[] = {ValPair, ValInt, ValAny};
  if (!CheckArgs(types, 3, num_args, vm)) return nil;

  Val list = StackPop(vm);
  Val index = StackPop(vm);
  Val item = StackPop(vm);

  Val new_list = ListTake(list, RawInt(index), &vm->mem);
  Val tail = Pair(item, ListFrom(list, RawInt(index), &vm->mem), &vm->mem);
  return ListConcat(new_list, tail, &vm->mem);
}

static Val PrimMapGet(u32 num_args, VM *vm)
{
  ValType types[] = {ValMap, ValAny};
  if (!CheckArgs(types, 2, num_args, vm)) return nil;

  Val map = StackPop(vm);
  Val key = StackPop(vm);
  return ValMapGet(map, key, &vm->mem);
}

static Val PrimMapPut(u32 num_args, VM *vm)
{
  ValType types[] = {ValMap, ValAny, ValAny};
  if (!CheckArgs(types, 3, num_args, vm)) return nil;

  Val map = StackPop(vm);
  Val key = StackPop(vm);
  Val value = StackPop(vm);
  return ValMapPut(map, key, value, &vm->mem);
}

static Val PrimMapDelete(u32 num_args, VM *vm)
{
  ValType types[] = {ValMap, ValAny};
  if (!CheckArgs(types, 2, num_args, vm)) return nil;

  Val map = StackPop(vm);
  Val key = StackPop(vm);
  return ValMapDelete(map, key, &vm->mem);
}

static Val PrimMapKeys(u32 num_args, VM *vm)
{
  ValType types[] = {ValMap};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Mem *mem = &vm->mem;
  Val map = StackPop(vm);
  Val keys = ValMapKeys(map, mem);
  Val tuple = MakeTuple(ValMapCount(map, mem), mem);
  for (u32 i = 0; i < ValMapCount(map, mem); i++) {
    TupleSet(tuple, i, TupleGet(keys, i, mem), mem);
  }
  return tuple;
}

static Val PrimMapValues(u32 num_args, VM *vm)
{
  ValType types[] = {ValMap};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Mem *mem = &vm->mem;
  Val map = StackPop(vm);
  Val vals = ValMapValues(map, mem);
  Val tuple = MakeTuple(ValMapCount(map, mem), mem);
  for (u32 i = 0; i < ValMapCount(map, mem); i++) {
    TupleSet(tuple, i, TupleGet(vals, i, mem), mem);
  }
  return tuple;
}

static Val PrimMapToList(u32 num_args, VM *vm)
{
  ValType types[] = {ValMap};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Mem *mem = &vm->mem;
  Val map = StackPop(vm);
  Val keys = ValMapKeys(map, mem);
  Val vals = ValMapValues(map, mem);
  Val list = nil;
  for (u32 i = 0; i < ValMapCount(map, mem); i++) {
    Val pair = Pair(TupleGet(keys, i, mem), TupleGet(vals, i, mem), mem);
    list = Pair(pair, list, mem);
  }
  return ReverseList(list, mem);
}
