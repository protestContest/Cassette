#include "system.h"
#include "univ/system.h"
#include "univ/math.h"
#include "mem/symbols.h"

Result SystemSet(void *context, Val key, Val value, Mem *mem)
{
  if (key == SymbolFor("seed")) {
    Seed(RawInt(value));
    return OkResult(Ok);
  } else {
    return OkResult(Nil);
  }
}

Result SystemGet(void *context, Val key, Mem *mem)
{
  if (key == SymbolFor("time")) {
    return OkResult(IntVal(Ticks()));
  } else if (key == SymbolFor("random")) {
    float r = (float)Random() / (float)MaxUInt;
    return OkResult(FloatVal(r));
  } else {
    return OkResult(Nil);
  }
}
