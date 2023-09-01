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
#include "builtin.h"

static PrimitiveDef stdlib[] = {
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
};

PrimitiveDef *StdLib(void)
{
  return stdlib;
}

u32 StdLibSize(void)
{
  return ArrayCount(stdlib);
}
