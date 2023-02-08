#include "printer.h"

u32 PrintVal(Val *mem, Symbol *symbols, Val value)
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
    return printf("%.*s", BinaryLength(mem, value), BinaryData(mem, value));
  } else if (IsSym(value)) {
    // return printf("s%d", RawObj(value));
    return printf(":%s", SymbolName(symbols, value));
  } else if (IsTuple(value)) {
    return printf("t%d", RawObj(value));
  } else if (IsDict(value)) {
    u32 count = printf("{");
    for (u32 i = 0; i < DictSize(mem, value); i++) {
      count += PrintVal(mem, symbols, DictKeyAt(mem, value, i));
      count += printf(": ");
      count += PrintVal(mem, symbols, DictValAt(mem, value, i));
      if (i != DictSize(mem, value) - 1) {
        count += printf(", ");
      }
    }
    count += printf("}");
    return count;
  } else {
    return 0;
  }
}

// #include "mem.h"
// #include "proc.h"

// void DebugVal(Val val)
// {
//   if (IsNil(val)) {
//     fprintf(stderr, " nil");
//   } else if (IsNum(val)) {
//     fprintf(stderr, "%-4.1f", val.as_f);
//   } else if (IsInt(val)) {
//     fprintf(stderr, "%-4d", RawInt(val));
//   } else if (IsPair(val)) {
//     fprintf(stderr, "p%3d", RawObj(val));
//   } else if (IsSym(val)) {
//     char *name = SymbolName(val);
//     fprintf(stderr, ":%s", name);
//   } else if (IsBin(val)) {
//     fprintf(stderr, "b%-3d", RawObj(val));
//   } else if (IsTuple(val)) {
//     fprintf(stderr, "t%-3d", RawObj(val));
//   } else if (IsDict(val)) {
//     fprintf(stderr, "m%-3d", RawObj(val));
//   } else {
//     fprintf(stderr, "?%-3d", val.as_v);
//   }
// }

// u32 PrintValTo(Val val, char *dst, u32 start, u32 size);

// u32 PrintTail(Val val, char *dst, u32 start, u32 size)
// {
//   start = PrintValTo(Head(val), dst, start, size);
//   if (IsNil(Tail(val))) {
//     return start;
//   } else if (IsPair(Tail(val))) {
//     start += snprintf(dst + start, size, " ");
//     return PrintTail(Tail(val), dst, start, size);
//   } else {
//     start += snprintf(dst + start, size, " | ");
//     return PrintValTo(Tail(val), dst, start, size);
//   }
// }

// u32 PrintValTo(Val val, char *dst, u32 start, u32 size)
// {
//   if (IsNum(val)) {
//     return start + snprintf(dst + start, size, "%.1f", val.as_f);
//   } else if (IsInt(val)) {
//     return start + snprintf(dst + start, size, "%d", RawInt(val));
//   } else if (IsNil(val)) {
//     return start + snprintf(dst + start, size, "nil");
//   } else if (IsTagged(val, "procedure")) {
//     start += snprintf(dst + start, size, "#[procedure (");

//     start = PrintValTo(ProcName(val), dst, start, size);
//     start += snprintf(dst + start, size, " ");

//     u32 env = ListLength(ProcEnv(val));

//     u32 count = ListLength(ProcParams(val));
//     for (u32 i = 0; i < count; i++) {
//       start = PrintValTo(ListAt(ProcParams(val), i), dst, start, size);
//       if (i != count-1) {
//         start += snprintf(dst + start, size, " ");
//       }
//     }
//     return start + snprintf(dst + start, size, ") <e%d>]", env);
//   } else if (IsPair(val)) {
//     start += snprintf(dst + start, size, "(");
//     start = PrintTail(val, dst, start, size);
//     return start + snprintf(dst + start, size, ")");
//   } else if (IsSym(val)) {
//     char *name = SymbolName(val);
//     return start + snprintf(dst + start, size, "%s", name);
//   } else if (IsBin(val)) {
//     u32 length = BinaryLength(val);
//     char *data = BinaryData(val);
//     if (size == 0) {
//       return start + length + 2;
//     } else {
//       start += snprintf(dst + start, size, "\"");
//       for (u32 i = 0; i < length; i++) {
//         start += snprintf(dst + start, size, "%c", data[i]);
//       }
//       start += snprintf(dst + start, size, "\"");
//       return start;
//     }
//   } else if (IsTuple(val)) {
//     start += snprintf(dst + start, size, "#[");
//     u32 count = TupleLength(val);
//     for (u32 i = 0; i < count; i++) {
//       start = PrintValTo(TupleAt(val, i), dst, start, size);
//       if (i != count-1) {
//         start += snprintf(dst + start, size, ", ");
//       }
//     }
//     return start + snprintf(dst + start, size, "]");
//   } else if (IsDict(val)) {
//     start += snprintf(dst + start, size, "{");

//     for (u32 i = 0; i < DICT_BUCKETS; i++) {
//       Val bucket = TupleAt(val, i);
//       while (!IsNil(bucket)) {
//         Val entry = Head(bucket);
//         Val var = Head(entry);
//         Val val = Tail(entry);
//         start = PrintValTo(var, dst, start, size);
//         start += snprintf(dst + start, size, ": ");
//         start = PrintValTo(val, dst, start, size);
//         start += snprintf(dst + start, size, ", ");
//         bucket = Tail(bucket);
//       }
//     }
//     return start + snprintf(dst + start, size, "}");
//   } else {
//     return start;
//   }
// }

// u32 ValStrLen(Val val)
// {
//   return PrintValTo(val, NULL, 0, 0);
// }

// char *ValStr(Val val)
// {
//   u32 length = ValStrLen(val) + 1;
//   char *str = malloc(length);
//   PrintValTo(val, str, 0, length);
//   str[length] = '\0';
//   return str;
// }

// char *ValAbbr(Val val)
// {
//   char *str = malloc(9);
//   PrintValTo(val, str, 0, 9);
//   str[9] = '\0';
//   return str;
// }

// void PrintVal(Val val)
// {
//   if (IsBin(val)) {
//     u32 length = BinaryLength(val);
//     // fprintf(stderr, "\"");
//     for (u32 i = 0; i < length; i++) {
//       fprintf(stderr, "%c", BinaryData(val)[i]);
//     }
//     // fprintf(stderr, "\"");
//     fprintf(stderr, "\n");
//   } else if (IsSym(val)) {
//     fprintf(stderr, ":%s\n", SymbolName(val));
//   } else {
//     char *str = ValStr(val);
//     fprintf(stderr, "%s\n", str);
//     free(str);
//   }
// }

// void Indent(u32 level, u32 lines, bool end)
// {
//   for (u32 i = 0; i < level; i++) {
//     u32 line = (0x1 << i) & lines;

//     if (i == level-1) {
//       if (end) {
//         fprintf(stderr, "└");
//       } else {
//         fprintf(stderr, "├");
//       }
//     } else if (line) {
//       fprintf(stderr, "│");
//     } else {
//       fprintf(stderr, " ");
//     }
//   }
// }

// void PrintTreeLevel(Val exp, u32 level, u32 lines);

// void PrintTreeTail(Val exp, u32 level, u32 lines)
// {
//   if (IsNil(Tail(exp))) {
//     Indent(level, lines, true);
//     lines = lines & ~(0x1 << (level-1));
//     PrintTreeLevel(Head(exp), level, lines);
//   } else {
//     Indent(level, lines, false);
//     PrintTreeLevel(Head(exp), level, lines);

//     if (IsPair(Tail(exp))) {
//       PrintTreeTail(Tail(exp), level, lines);
//     } else {
//       Indent(level, lines, true);
//       PrintTreeLevel(Tail(exp), level, lines);
//     }
//   }
// }

// void PrintTreeLevel(Val exp, u32 level, u32 lines)
// {
//   if (IsList(exp)) {
//     fprintf(stderr, "┬");
//     lines = (0x1 << level) | lines;
//     level++;
//     PrintTreeLevel(Head(exp), level, lines);

//     if (!IsNil(Tail(exp))) {
//       PrintTreeTail(Tail(exp), level, lines);
//     }
//   } else {
//     fprintf(stderr, "╴");
//     if (IsNil(exp)) {
//       fprintf(stderr, "nil\n");
//     } else if (IsPair(exp)) {
//       fprintf(stderr, "[%s | %s ]\n", ValStr(Head(exp)), ValStr(Tail(exp)));
//     } else {
//       DebugVal(exp);
//       fprintf(stderr, "\n");
//     }
//   }
// }

// void PrintTree(Val exp)
// {
//   Indent(0, 0, false);
//   PrintTreeLevel(exp, 0, 0);
//   fprintf(stderr, "\n");
// }
