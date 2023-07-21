#include "primitives.h"
#include "env.h"
#include "function.h"

static Val PrimPrint(u32 num_args, VM *vm);
static Val PrimInspect(u32 num_args, VM *vm);
static Val PrimRandom(u32 num_args, VM *vm);
static Val PrimIsInt(u32 num_args, VM *vm);
static Val PrimIsFloat(u32 num_args, VM *vm);
static Val PrimIsNumber(u32 num_args, VM *vm);
static Val PrimIsSymbol(u32 num_args, VM *vm);
static Val PrimIsList(u32 num_args, VM *vm);
static Val PrimIsTuple(u32 num_args, VM *vm);
static Val PrimIsBinary(u32 num_args, VM *vm);
static Val PrimIsMap(u32 num_args, VM *vm);
static Val PrimRemainder(u32 num_args, VM *vm);
static Val PrimDiv(u32 num_args, VM *vm);
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
// static Val PrimListTake(u32 num_args, VM *vm);
// static Val PrimListSlice(u32 num_args, VM *vm);
// static Val PrimListConcat(u32 num_args, VM *vm);
// static Val PrimListAppend(u32 num_args, VM *vm);
// static Val PrimListFlatten(u32 num_args, VM *vm);
static Val PrimListToBinary(u32 num_args, VM *vm);
static Val PrimListToTuple(u32 num_args, VM *vm);
static Val PrimListToMap(u32 num_args, VM *vm);
// static Val PrimListInsert(u32 num_args, VM *vm);
// static Val PrimListZip(u32 num_args, VM *vm);
// static Val PrimListUnzip(u32 num_args, VM *vm);
static Val PrimMapNew(u32 num_args, VM *vm);
static Val PrimMapGet(u32 num_args, VM *vm);
static Val PrimMapPut(u32 num_args, VM *vm);
static Val PrimMapDelete(u32 num_args, VM *vm);
static Val PrimMapKeys(u32 num_args, VM *vm);
static Val PrimMapValues(u32 num_args, VM *vm);
static Val PrimMapToList(u32 num_args, VM *vm);
static Val PrimTupleToList(u32 num_args, VM *vm);
static Val PrimFileRead(u32 num_args, VM *vm);
static Val PrimFileWrite(u32 num_args, VM *vm);
static Val PrimFileExists(u32 num_args, VM *vm);

static struct {char *mod; char *name; PrimitiveFn fn;} primitives[] = {
  {NULL, "print", &PrimPrint},
  {NULL, "inspect", &PrimInspect},
  {NULL, "random", &PrimRandom},
  {NULL, "is-int", &PrimIsInt},
  {NULL, "is-float", &PrimIsFloat},
  {NULL, "is-number", &PrimIsNumber},
  {NULL, "is-symbol", &PrimIsSymbol},
  {NULL, "is-list", &PrimIsList},
  {NULL, "is-tuple", &PrimIsTuple},
  {NULL, "is-binary", &PrimIsBinary},
  {NULL, "is-map", &PrimIsMap},
  {"Math", "rem", &PrimRemainder},
  {"Math", "div", &PrimDiv},
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
  // {"List", "take", &PrimListTake},
  // {"List", "slice", &PrimListSlice},
  // {"List", "concat", &PrimListConcat},
  // {"List", "append", &PrimListAppend},
  // {"List", "flatten", &PrimListFlatten},
  {"List", "to-binary", &PrimListToBinary},
  {"List", "to-tuple", &PrimListToTuple},
  {"List", "to-map", &PrimListToMap},
  // {"List", "insert", &PrimListInsert},
  // {"List", "zip", &PrimListZip},
  // {"List", "unzip", &PrimListUnzip},
  {"Map", "new", &PrimMapNew},
  {"Map", "get", &PrimMapGet},
  {"Map", "put", &PrimMapPut},
  {"Map", "delete", &PrimMapDelete},
  {"Map", "keys", &PrimMapKeys},
  {"Map", "values", &PrimMapValues},
  {"Map", "to-list", &PrimMapToList},
  {"Tuple", "to_list", &PrimTupleToList},
  {"File", "read", &PrimFileRead},
  {"File", "write", &PrimFileWrite},
  {"File", "exists?", &PrimFileExists},
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
  HashMap modules;
  InitHashMap(&modules);

  // get a count of primitives + constants for each module
  for (u32 i = 0; i < ArrayCount(primitives); i++) {
    if (primitives[i].mod == NULL) continue;
    Val mod_name = MakeSymbol(primitives[i].mod, mem);
    u32 count = 0;
    if (HashMapContains(&modules, mod_name.as_i)) {
      count = HashMapGet(&modules, mod_name.as_i);
    }
    HashMapSet(&modules, mod_name.as_i, count + 1);
  }
  for (u32 i = 0; i < ArrayCount(constants); i++) {
    if (constants[i].mod == NULL) continue;
    Val mod_name = MakeSymbol(constants[i].mod, mem);
    u32 count = 0;
    if (HashMapContains(&modules, mod_name.as_i)) {
      count = HashMapGet(&modules, mod_name.as_i);
    }
    HashMapSet(&modules, mod_name.as_i, count + 1);
  }

  // create each module
  for (u32 i = 0; i < HashMapCount(&modules); i++) {
    Val mod_name = (Val){.as_i = GetHashMapKey(&modules, i)};
    u32 count = HashMapGet(&modules, mod_name.as_i);
    Val module = MakeMap(count, mem);
    Define(mod_name, module, env, mem);
    HashMapSet(&modules, mod_name.as_i, module.as_i);
  }

  // define primitives for each module
  for (u32 i = 0; i < ArrayCount(primitives); i++) {
    Val primitive_name = MakeSymbol(primitives[i].name, mem);
    Val func = MakePrimitive(IntVal(i), mem);

    if (primitives[i].mod == NULL) {
      Define(primitive_name, func, env, mem);
    } else {
      Val mod_name = SymbolFor(primitives[i].mod);
      Val module = (Val){.as_i = HashMapGet(&modules, mod_name.as_i)};
      MapSet(module, primitive_name, func, mem);
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
      Val module = (Val){.as_i = HashMapGet(&modules, mod_name.as_i)};
      MapSet(module, const_name, value, mem);
    }
  }

  DestroyHashMap(&modules);
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
  case ValMap:      return IsMap(arg, mem);
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

static Val PrimIsInt(u32 num_args, VM *vm)
{
  ValType types[] = {ValAny};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Val arg = StackPop(vm);
  return BoolVal(IsInt(arg));
}

static Val PrimIsFloat(u32 num_args, VM *vm)
{
  ValType types[] = {ValAny};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Val arg = StackPop(vm);
  return BoolVal(IsNum(arg));
}

static Val PrimIsNumber(u32 num_args, VM *vm)
{
  ValType types[] = {ValAny};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Val arg = StackPop(vm);
  return BoolVal(IsNumeric(arg));
}

static Val PrimIsSymbol(u32 num_args, VM *vm)
{
  ValType types[] = {ValAny};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Val arg = StackPop(vm);
  return BoolVal(IsSym(arg));
}

static Val PrimIsList(u32 num_args, VM *vm)
{
  ValType types[] = {ValAny};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Val arg = StackPop(vm);
  return BoolVal(IsPair(arg));
}

static Val PrimIsTuple(u32 num_args, VM *vm)
{
  ValType types[] = {ValAny};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Val arg = StackPop(vm);
  return BoolVal(IsTuple(arg, &vm->mem));
}

static Val PrimIsBinary(u32 num_args, VM *vm)
{
  ValType types[] = {ValAny};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Val arg = StackPop(vm);
  return BoolVal(IsBinary(arg, &vm->mem));
}

static Val PrimIsMap(u32 num_args, VM *vm)
{
  ValType types[] = {ValAny};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Val arg = StackPop(vm);
  return BoolVal(IsMap(arg, &vm->mem));
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

static Val PrimDiv(u32 num_args, VM *vm)
{
  ValType types[] = {ValInt, ValInt};
  if (!CheckArgs(types, 2, num_args, vm)) return nil;

  Val a = StackPop(vm);
  Val b = StackPop(vm);

  return IntVal(RawInt(a) / RawInt(b));
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

// static Val PrimListTake(u32 num_args, VM *vm)
// {
//   ValType types[] = {ValPair, ValInt};
//   if (!CheckArgs(types, 2, num_args, vm)) return nil;

//   Val list = StackPop(vm);
//   Val n = StackPop(vm);

//   return ListTake(list, RawInt(n), &vm->mem);
// }

// static Val PrimListSlice(u32 num_args, VM *vm)
// {
//   ValType types[] = {ValPair, ValInt, ValInt};
//   if (!CheckArgs(types, 3, num_args, vm)) return nil;

//   Val list = StackPop(vm);
//   Val start = StackPop(vm);
//   Val end = StackPop(vm);

//   return ListTake(ListFrom(list, RawInt(start), &vm->mem), RawInt(end), &vm->mem);
// }

// static Val PrimListConcat(u32 num_args, VM *vm)
// {
//   ValType types[] = {ValPair, ValPair};
//   if (!CheckArgs(types, 2, num_args, vm)) return nil;

//   Val a = StackPop(vm);
//   Val b = StackPop(vm);
//   Val list = ListTake(a, ListLength(a, &vm->mem), &vm->mem);
//   return ListConcat(list, b, &vm->mem);
// }

// static Val PrimListAppend(u32 num_args, VM *vm)
// {
//   ValType types[] = {ValPair, ValAny};
//   if (!CheckArgs(types, 2, num_args, vm)) return nil;

//   Val list = StackPop(vm);
//   Val item = StackPop(vm);
//   list = ListTake(list, ListLength(list, &vm->mem), &vm->mem);
//   return ListConcat(list, Pair(item, nil, &vm->mem), &vm->mem);
// }

// static Val PrimListFlatten(u32 num_args, VM *vm)
// {
//   ValType types[] = {ValPair};
//   if (!CheckArgs(types, 1, num_args, vm)) return nil;

//   Val list = StackPop(vm);
//   return ListFlatten(list, &vm->mem);
// }

static Val PrimListToBinary(u32 num_args, VM *vm)
{
  ValType types[] = {ValPair};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Val list = StackPop(vm);
  return ListToBinary(list, &vm->mem);
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
  Val map = MakeMap(count, &vm->mem);
  while (!IsNil(list)) {
    Val item = Head(list, &vm->mem);

    if (!IsPair(item)) {
      RuntimeError(vm, "Could not convert list to map");
      return nil;
    }

    MapSet(map, Head(item, &vm->mem), Tail(item, &vm->mem), &vm->mem);

    list = Tail(list, &vm->mem);
  }

  return map;
}

// static Val PrimListInsert(u32 num_args, VM *vm)
// {
//   ValType types[] = {ValPair, ValInt, ValAny};
//   if (!CheckArgs(types, 3, num_args, vm)) return nil;

//   Val list = StackPop(vm);
//   Val index = StackPop(vm);
//   Val item = StackPop(vm);

//   Val new_list = ListTake(list, RawInt(index), &vm->mem);
//   Val tail = Pair(item, ListFrom(list, RawInt(index), &vm->mem), &vm->mem);
//   return ListConcat(new_list, tail, &vm->mem);
// }

// static Val PrimListZip(u32 num_args, VM *vm)
// {
//   ValType types[] = {ValPair, ValPair};
//   if (!CheckArgs(types, 2, num_args, vm)) return nil;

//   Mem *mem = &vm->mem;
//   Val heads = StackPop(vm);
//   Val tails = StackPop(vm);
//   Val zipped = nil;

//   while (!IsNil(heads)) {
//     Val head = Head(heads, mem);
//     Val tail = Head(tails, mem);
//     zipped = Pair(Pair(head, tail, mem), zipped, mem);
//     heads = Tail(heads, mem);
//     tails = Tail(tails, mem);
//   }
//   while (!IsNil(tails)) {
//     Val tail = Head(tails, mem);
//     zipped = Pair(Pair(nil, tail, mem), zipped, mem);
//     tails = Tail(tails, mem);
//   }

//   return zipped;
// }

// static Val PrimListUnzip(u32 num_args, VM *vm)
// {
//   ValType types[] = {ValPair};
//   if (!CheckArgs(types, 1, num_args, vm)) return nil;

//   Mem *mem = &vm->mem;
//   Val zipped = StackPop(vm);
//   Val heads = nil;
//   Val tails = nil;
//   while (!IsNil(zipped)) {
//     Val item = Head(zipped, mem);
//     if (IsPair(item)) {
//       heads = Pair(Head(item, mem), heads, mem);
//       tails = Pair(Tail(item, mem), tails, mem);
//     } else {
//       heads = Pair(item, heads, mem);
//       tails = Pair(nil, tails, mem);
//     }
//     zipped = Tail(zipped, mem);
//   }

//   return Pair(heads, tails, mem);
// }

static Val PrimMapNew(u32 num_args, VM *vm)
{
  if (!CheckArgs(NULL, 0, num_args, vm)) return nil;
  return MakeMap(0, &vm->mem);
}

static Val PrimMapGet(u32 num_args, VM *vm)
{
  ValType types[] = {ValMap, ValAny};
  if (!CheckArgs(types, 2, num_args, vm)) return nil;

  Val map = StackPop(vm);
  Val key = StackPop(vm);
  return MapGet(map, key, &vm->mem);
}

static Val PrimMapPut(u32 num_args, VM *vm)
{
  ValType types[] = {ValMap, ValAny, ValAny};
  if (!CheckArgs(types, 3, num_args, vm)) return nil;

  Val map = StackPop(vm);
  Val key = StackPop(vm);
  Val value = StackPop(vm);
  return MapPut(map, key, value, &vm->mem);
}

static Val PrimMapDelete(u32 num_args, VM *vm)
{
  ValType types[] = {ValMap, ValAny};
  if (!CheckArgs(types, 2, num_args, vm)) return nil;

  Val map = StackPop(vm);
  Val key = StackPop(vm);
  return MapDelete(map, key, &vm->mem);
}

static Val PrimMapKeys(u32 num_args, VM *vm)
{
  ValType types[] = {ValMap};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Mem *mem = &vm->mem;
  Val map = StackPop(vm);
  Val keys = MapKeys(map, mem);
  Val tuple = MakeTuple(MapCount(map, mem), mem);
  for (u32 i = 0; i < MapCount(map, mem); i++) {
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
  Val vals = MapValues(map, mem);
  Val tuple = MakeTuple(MapCount(map, mem), mem);
  for (u32 i = 0; i < MapCount(map, mem); i++) {
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
  Val keys = MapKeys(map, mem);
  Val vals = MapValues(map, mem);
  Val list = nil;
  for (u32 i = 0; i < MapCount(map, mem); i++) {
    Val pair = Pair(TupleGet(keys, i, mem), TupleGet(vals, i, mem), mem);
    list = Pair(pair, list, mem);
  }
  return ReverseList(list, mem);
}

static Val PrimTupleToList(u32 num_args, VM *vm)
{
  ValType types[] = {ValTuple};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Mem *mem = &vm->mem;
  Val tuple = StackPop(vm);
  Val list = nil;

  for (u32 i = 0; i < TupleLength(tuple, mem); i++) {
    list = Pair(TupleGet(tuple, i, mem), list, mem);
  }

  return ReverseList(list, mem);
}

static Val PrimFileRead(u32 num_args, VM *vm)
{
  ValType types[] = {ValBinary};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Mem *mem = &vm->mem;
  Val filename = StackPop(vm);
  u32 name_length = BinaryLength(filename, mem);
  char path[name_length + 1];
  Copy(BinaryData(filename, mem), path, name_length);
  path[name_length] = '\0';

  if (!FileExists(path)) return MakeSymbol("error", mem);

  char *data = ReadFile(path);
  return Pair(MakeSymbol("ok", mem), BinaryFrom(data, mem), mem);
}

static Val PrimFileWrite(u32 num_args, VM *vm)
{
  ValType types[] = {ValBinary, ValBinary};
  if (!CheckArgs(types, 2, num_args, vm)) return nil;

  Mem *mem = &vm->mem;
  Val filename = StackPop(vm);
  u32 name_length = BinaryLength(filename, mem);
  char path[name_length + 1];
  Copy(BinaryData(filename, mem), path, name_length);
  path[name_length] = '\0';

  Val data = StackPop(vm);

  if (WriteFile(path, BinaryData(data, mem), BinaryLength(data, mem))) {
    return MakeSymbol("ok", mem);
  } else {
    return MakeSymbol("error", mem);
  }
}

static Val PrimFileExists(u32 num_args, VM *vm)
{
  ValType types[] = {ValBinary};
  if (!CheckArgs(types, 1, num_args, vm)) return nil;

  Mem *mem = &vm->mem;
  Val filename = StackPop(vm);
  u32 name_length = BinaryLength(filename, mem);
  char path[name_length + 1];
  Copy(BinaryData(filename, mem), path, name_length);
  path[name_length] = '\0';

  return BoolVal(FileExists(path));
}
