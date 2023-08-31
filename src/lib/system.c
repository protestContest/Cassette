#include "system.h"

Val SysTime(VM *vm, Val args)
{
  return IntVal(Time());
}

Val SysExit(VM *vm, Val args)
{
  vm->cont = MaxUInt;
  return SymbolFor("ok");
}
