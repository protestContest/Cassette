#include "print.h"
#include "../value.h"
#include "mem.h"
#include "../image/string_map.h"
#include <univ/io.h>
#include <univ/str.h>

u32 PrintVal(Val *mem, Val symbols, Val value)
{
  if (IsNil(value)) {
    Print("nil");
    return 3;
  } else if (IsNum(value)) {
    PrintFloat(value.as_f);
    return NumDigits(value.as_f);
  } else if (IsInt(value)) {
    PrintInt(RawInt(value), 0);
    return NumDigits(RawInt(value));
  } else if (IsSym(value)) {
    if (!IsNil(symbols)) {
      Print(":");
      Val bin = MapGet(mem, symbols, value);
      Append(output, BinaryData(mem, bin), BinaryLength(mem, bin));
      return BinaryLength(mem, bin) + 1;
    } else {
      Print("<sym>");
      return 5;
    }
  } else if (IsPair(value)) {
    Print("p");
    PrintInt(RawPair(value), 0);
    return NumDigits(RawPair(value)) + 1;
  } else {
    Print("<v");
    PrintInt(RawObj(value), 4);
    Print(">");
    return 7;
  }
}
