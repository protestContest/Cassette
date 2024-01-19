#include "system.h"
#include "runtime/mem/symbols.h"
#include "univ/font.h"
#include "univ/math.h"
#include "univ/serial.h"
#include "univ/str.h"
#include "univ/system.h"

#define SymSeed         0x7FDCADD1 /* seed */
#define SymRandom       0x7FD3FCF1 /* random */
#define SymTime         0x7FD30526 /* time */
#define SymSerialPorts  0x7FD04B3C /* serial-ports */
#define SymFonts        0x7FDB6110 /* fonts */

Result SystemSet(void *context, Val key, Val value, Mem *mem)
{
  if (key == SymSeed) {
    Seed(RawInt(value));
    return ValueResult(Ok);
  } else {
    return ValueResult(Nil);
  }
}

Result SystemGet(void *context, Val key, Mem *mem)
{
  if (key == SymTime) {
    return ValueResult(IntVal(Time()));
  } else if (key == SymRandom) {
    float r = (float)Random() / (float)MaxUInt;
    return ValueResult(FloatVal(r));
  } else if (key == SymSerialPorts) {
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
    return ValueResult(list);
  } else if (key == SymFonts) {
    ObjVec *fonts = ListFonts();
    Val list = Nil;
    u32 i;
    for (i = 0; i < fonts->count; i++) {
      FontInfo *info = VecRef(fonts, i);
      Val name = BinaryFrom(info->name, StrLen(info->name), mem);
      list = Pair(name, list, mem);
      Free(info);
    }
    DestroyVec(fonts);
    Free(fonts);
    return ValueResult(ReverseList(list, Nil, mem));
  } else {
    return ValueResult(Nil);
  }
}
