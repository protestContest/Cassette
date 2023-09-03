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
#include "canvas.h"

static PrimitiveDef stdlib[] = {
  {NULL, "head", StdHead},
  {NULL, "tail", StdTail},
  {NULL, "error", StdError},
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
  {NULL, "random", MathRandom},
  {NULL, "rand_int", MathRandInt},
  {NULL, "ceil", MathCeil},
  {NULL, "floor", MathFloor},
  {NULL, "round", MathRound},
  {NULL, "abs", MathAbs},
  {NULL, "max", MathMax},
  {NULL, "min", MathMin},
  {NULL, "sin", MathSin},
  {NULL, "cos", MathCos},
  {NULL, "tan", MathTan},
  {NULL, "ln", MathLn},
  {NULL, "exp", MathExp},
  {NULL, "pow", MathPow},
  {NULL, "sqrt", MathSqrt},
  {NULL, "Pi", MathPi},
  {NULL, "E", MathE},
  {NULL, "Infinity", MathInfinity},
  {NULL, "bit_and", BitAnd},
  {NULL, "bit_or", BitOr},
  {NULL, "bit_not", BitNot},
  {NULL, "bit_xor", BitXOr},
  {NULL, "bit_shift", BitShift},
  {NULL, "bit_count", BitCount},
  {"System", "time", SysTime},
  {"System", "exit", SysExit},
#ifdef WITH_CANVAS
  {"Canvas", "width", CanvasWidth},
  {"Canvas", "height", CanvasHeight},
  {"Canvas", "line", CanvasLine},
#endif
};

PrimitiveDef *StdLib(void)
{
  return stdlib;
}

u32 StdLibSize(void)
{
  return ArrayCount(stdlib);
}
