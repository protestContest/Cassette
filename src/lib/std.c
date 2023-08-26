#include "std.h"
#include "io.h"
#include "list.h"
#include "tuple.h"
#include "map.h"

// fake a Vec so we can use VecCount
static struct {u32 cap; u32 count; PrimitiveDef items[];} stdlib = {
  21, 21, {
    {NULL, "head", StdHead},
    {NULL, "tail", StdTail},
    {NULL, "nil?", StdIsNil},
    {NULL, "number?", StdIsNum},
    {NULL, "integer?", StdIsInt},
    {NULL, "symbol?", StdIsSym},
    {NULL, "pair?", StdIsPair},
    {NULL, "tuple?", StdIsTuple},
    {NULL, "binary?", StdIsBinary},
    {NULL, "map?", StdIsMap},
    {NULL, "inspect", IOInspect},
    {"IO", "open", IOOpen},
    {"IO", "read", IORead},
    {"IO", "write", IOWrite},
    {"IO", "read_file", IOReadFile},
    {"IO", "write_file", IOWriteFile},
    {"List", "to_binary", ListToBin},
    {"List", "to_tuple", ListToTuple},
    {"Tuple", "to_list", TupleToList},
    {"Map", "keys", StdMapKeys},
    {"Map", "values", StdMapValues},
  }
};

PrimitiveDef *StdLib(void)
{
  return stdlib.items;
}

Val StdHead(VM *vm, Val args)
{
  Val pair = TupleGet(args, 0, vm->mem);
  if (IsPair(pair)) {
    return Head(pair, vm->mem);
  } else {
    vm->error = TypeError;
    return nil;
  }
}

Val StdTail(VM *vm, Val args)
{
  Val pair = TupleGet(args, 0, vm->mem);
  if (IsPair(pair)) {
    return Tail(pair, vm->mem);
  } else {
    vm->error = TypeError;
    return nil;
  }
}

Val StdIsNil(VM *vm, Val args)
{
  return BoolVal(IsNil(TupleGet(args, 0, vm->mem)));
}

Val StdIsNum(VM *vm, Val args)
{
  return BoolVal(IsNum(TupleGet(args, 0, vm->mem)));
}

Val StdIsInt(VM *vm, Val args)
{
  return BoolVal(IsInt(TupleGet(args, 0, vm->mem)));
}

Val StdIsSym(VM *vm, Val args)
{
  return BoolVal(IsSym(TupleGet(args, 0, vm->mem)));
}

Val StdIsPair(VM *vm, Val args)
{
  return BoolVal(IsPair(TupleGet(args, 0, vm->mem)));
}

Val StdIsTuple(VM *vm, Val args)
{
  return BoolVal(IsTuple(TupleGet(args, 0, vm->mem), vm->mem));
}

Val StdIsBinary(VM *vm, Val args)
{
  return BoolVal(IsBinary(TupleGet(args, 0, vm->mem), vm->mem));
}

Val StdIsMap(VM *vm, Val args)
{
  return BoolVal(IsMap(TupleGet(args, 0, vm->mem), vm->mem));
}

