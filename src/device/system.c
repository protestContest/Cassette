#include "system.h"
#include "univ/system.h"
#include "univ/math.h"
#include "mem/symbols.h"

#define SymSeed           0x7FDCADD1 /* seed */
#define SymRandom         0x7FD3FCF1 /* random */
#define SymTime           0x7FD30526 /* time */

Result SystemSet(void *context, Val key, Val value, Mem *mem)
{
  if (key == SymSeed) {
    Seed(RawInt(value));
    return OkResult(Ok);
  } else {
    return OkResult(Nil);
  }
}

Result SystemGet(void *context, Val key, Mem *mem)
{
  if (key == SymTime) {
    return OkResult(IntVal(Ticks()));
  } else if (key == SymRandom) {
    float r = (float)Random() / (float)MaxUInt;
    return OkResult(FloatVal(r));
  } else {
    return OkResult(Nil);
  }
}
