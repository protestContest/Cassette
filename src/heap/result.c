#include "result.h"
#include "tuple.h"
#include "symbol.h"

Val OkResult(Val value, Heap *mem)
{
  Val result = MakeTuple(2, mem);
  TupleSet(result, 0, Ok, mem);
  TupleSet(result, 1, value, mem);
  return result;
}

Val ErrorResult(char *error, Heap *mem)
{
  Val result = MakeTuple(2, mem);
  TupleSet(result, 0, Error, mem);
  TupleSet(result, 1, MakeSymbol(error, mem), mem);
  return result;
}
