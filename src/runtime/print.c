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
    PrintInt(RawVal(value), 0);
    return NumDigits(RawVal(value)) + 1;
  } else if (IsBin(value)) {
    Append(output, BinaryData(mem, value), BinaryLength(mem, value));
    return BinaryLength(mem, value);
  } else if (IsTuple(value)) {
    Print("t");
    PrintInt(RawVal(value), 0);
    return NumDigits(RawVal(value)) + 1;
  } else if (IsMap(value)) {
    Print("{");
    u32 count = 1;
    for (u32 i = 0; i < MapSize(mem, value); i++) {
      Val key = MapKeyAt(mem, value, i);

      if (!IsNil(symbols)) {
        Val bin = MapGet(mem, symbols, key);
        Append(output, BinaryData(mem, bin), BinaryLength(mem, bin));
        count += BinaryLength(mem, bin);
      } else {
        Print("<sym>");
        count += 5;
      }

      Print(": ");
      count += 2;
      count += PrintVal(mem, symbols, MapValAt(mem, value, i));
      if (i != MapSize(mem, value) - 1) {
        Print(", ");
        count += 2;
      }
    }
    Print("}");
    count += 1;
    return count;
  } else {
    Print("<v");
    PrintInt(RawVal(value), 4);
    Print(">");
    return 7;
  }
}
