#include "stdlib.h"

static PrimitiveDef primitives[0] = {};

u32 StdLibSize(void)
{
  return 0;
}

PrimitiveDef *StdLib(void)
{
  return primitives;
}
