#include "printer.h"

u32 PrintVal(Val *mem, StringMap *strings, Val value)
{
  if (IsNil(value)) {
    return printf("nil");
  } else if (IsPair(value)) {
    return printf("p%d", RawObj(value));
  } else if (IsNum(value)) {
    return printf("%.1f", value.as_f);
  } else if (IsInt(value)) {
    return printf("%d", RawInt(value));
  } else if (IsBin(value)) {
    return printf("%.*s", StringLength(strings, value), StringData(strings, value));
  } else if (IsSym(value)) {
    return printf(":%s", SymbolName(strings, value));
  } else if (IsTuple(value)) {
    return printf("t%d", RawObj(value));
  } else if (IsMap(value)) {
    u32 count = printf("{");
    for (u32 i = 0; i < MapSize(mem, value); i++) {
      count += printf("%s", SymbolName(strings, MapKeyAt(mem, value, i)));
      count += printf(": ");
      count += PrintVal(mem, strings, MapValAt(mem, value, i));
      if (i != MapSize(mem, value) - 1) {
        count += printf(", ");
      }
    }
    count += printf("}");
    return count;
  } else {
    return printf("<value>");
  }
}
