#include "system.h"
#include "univ/system.h"
#include "univ/math.h"
#include "univ/serial.h"
#include "univ/str.h"
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
    return OkResult(IntVal(Time()));
  } else if (key == SymbolFor("ticks")) {
    return OkResult(IntVal(Ticks()));
  } else if (key == SymRandom) {
    float r = (float)Random() / (float)MaxUInt;
    return OkResult(FloatVal(r));
  } else if (key == SymbolFor("serial-ports")) {
    ObjVec *ports = ListSerialPorts();
    u32 i;
    Val list = Nil;
    for (i = 0; i < ports->count; i++) {
      SerialPort *port = (SerialPort*)ports->items[i];
      Val path = BinaryFrom(port->path, StrLen(port->path), mem);
      Val name = BinaryFrom(port->name, StrLen(port->name), mem);
      list = Pair(Pair(name, path, mem), list, mem);
      if (port->path) Free(port->path);
      if (port->name) Free(port->name);
      Free(port);
    }
    list = ReverseList(list, Nil, mem);
    Free(ports);
    return OkResult(list);
  } else {
    return OkResult(Nil);
  }
}
