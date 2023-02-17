#include "print.h"
#include "mem.h"
#include "../image/string_map.h"
#include "../platform/console.h"

u32 PrintVal(Val *mem, StringMap *strings, Val value)
{
  if (IsNil(value)) {
    return Print("nil");
  } else if (IsPair(value)) {
    return Print("p%d", RawObj(value));
  } else if (IsNum(value)) {
    return Print("%.1f", value.as_f);
  } else if (IsInt(value)) {
    return Print("%d", RawInt(value));
  } else if (IsBin(value)) {
    return Print("%.*s", StringLength(strings, value), StringData(strings, value));
  } else if (IsSym(value)) {
    return Print(":%s", SymbolName(strings, value));
  } else if (IsTuple(value)) {
    return Print("t%d", RawObj(value));
  } else if (IsMap(value)) {
    u32 count = Print("{");
    for (u32 i = 0; i < MapSize(mem, value); i++) {
      count += Print("%s", SymbolName(strings, MapKeyAt(mem, value, i)));
      count += Print(": ");
      count += PrintVal(mem, strings, MapValAt(mem, value, i));
      if (i != MapSize(mem, value) - 1) {
        count += Print(", ");
      }
    }
    count += Print("}");
    return count;
  } else {
    return Print("<value>");
  }
}
