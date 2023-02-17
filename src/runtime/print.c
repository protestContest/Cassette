#include "print.h"
#include "mem.h"
#include "../image/string_map.h"
#include "../console.h"

u32 PrintVal(Val *mem, StringMap *strings, Val value)
{
  if (IsNil(value)) {
    Print("nil");
    return 3;
  } else if (IsPair(value)) {
    Print("p");
    PrintInt(RawObj(value), 0);
    return NumDigits(RawObj(value)) + 1;
  } else if (IsNum(value)) {
    PrintFloat(value.as_f);
    return NumDigits(value.as_f);
  } else if (IsInt(value)) {
    PrintInt(RawInt(value), 0);
    return NumDigits(RawInt(value));
  } else if (IsBin(value)) {
    Append(outbuf, (u8*)StringData(strings, value), StringLength(strings, value));
    return StringLength(strings, value);
  } else if (IsSym(value)) {
    Print(":");
    Print(SymbolName(strings, value));
    return StrLen(SymbolName(strings, value)) + 1;
  } else if (IsTuple(value)) {
    Print("t");
    PrintInt(RawObj(value), 0);
    return NumDigits(RawObj(value)) + 1;
  } else if (IsMap(value)) {
    Print("{");
    u32 count = 1;
    for (u32 i = 0; i < MapSize(mem, value); i++) {
      char *key = SymbolName(strings, MapKeyAt(mem, value, i));
      Print(key);
      count += StrLen(key);
      Print(": ");
      count += 2;
      count += PrintVal(mem, strings, MapValAt(mem, value, i));
      if (i != MapSize(mem, value) - 1) {
        Print(", ");
        count += 2;
      }
    }
    Print("}");
    count += 1;
    return count;
  } else {
    Print("<value>");
    return 7;
  }
}
