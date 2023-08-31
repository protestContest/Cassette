#include "std.h"
#include "io.h"
#include "list.h"
#include "tuple.h"
#include "map.h"
#include "binary.h"
#include "bit.h"
#include "system.h"
#include "math.h"
#include "string.h"

// fake a Vec so we can use VecCount
static struct {u32 cap; u32 count; PrimitiveDef items[];} stdlib = {
  26, 26, {
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
    {NULL, "print", IOPrint},
    {"IO", "open", IOOpen},
    {"IO", "read", IORead},
    {"IO", "write", IOWrite},
    {"IO", "read_file", IOReadFile},
    {"IO", "write_file", IOWriteFile},
    {"List", "to_binary", ListToBin},
    {"List", "to_tuple", ListToTuple},
    {"List", "reverse", ListReverse},
    {"List", "trunc", ListTrunc},
    {"List", "tail", ListAfter},
    {"List", "join", ListJoin},
    {"Tuple", "to_list", TupleToList},
    {"Binary", "to_list", BinToList},
    {"Binary", "trunc", BinTrunc},
    {"Binary", "after", BinAfter},
    {"Binary", "slice", BinSlice},
    {"Binary", "join", BinJoin},
    {"Map", "new", StdMapNew},
    {"Map", "keys", StdMapKeys},
    {"Map", "values", StdMapValues},
    {"Map", "put", StdMapPut},
    {"Map", "get", StdMapGet},
    {"Map", "delete", StdMapDelete},
    {"Map", "to_list", StdMapToList},
    {"Math", "random", MathRandom},
    {"Math", "rand_int", MathRandInt},
    {"Math", "ceil", MathCeil},
    {"Math", "floor", MathFloor},
    {"Math", "round", MathRound},
    {"Math", "abs", MathAbs},
    {"Math", "max", MathMax},
    {"Math", "min", MathMin},
    {"Math", "sin", MathSin},
    {"Math", "cos", MathCos},
    {"Math", "tan", MathTan},
    {"Math", "ln", MathLn},
    {"Math", "exp", MathExp},
    {"Math", "pow", MathPow},
    {"Math", "sqrt", MathSqrt},
    {"Math", "Pi", MathPi},
    {"Math", "E", MathE},
    {"Math", "Infinity", MathInfinity},
    {"Bit", "and", BitAnd},
    {"Bit", "or", BitOr},
    {"Bit", "not", BitNot},
    {"Bit", "xor", BitXOr},
    {"Bit", "shift", BitShift},
    {"Bit", "count", BitCount},
    {"String", "valid?", StringValid},
    {"String", "length", StringLength},
    {"String", "at", StringAt},
    {"String", "slice", StringSlice},
    {"System", "time", SysTime},
    {"System", "exit", SysExit},
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

