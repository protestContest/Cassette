#include "mem.h"
#include <stdarg.h>
#include <univ/vec.h>
#include <univ/hash.h>
#include <univ/io.h>
#include <univ/str.h>
#include <univ/allocate.h>

void InitMem(Mem *mem)
{
  mem->values = NULL;
  VecPush(mem->values, nil);
  VecPush(mem->values, nil);
  InitMap(&mem->symbols);
}

Val MakePair(Mem *mem, Val head, Val tail)
{
  Val pair = PairVal(VecCount(mem->values));

  VecPush(mem->values, head);
  VecPush(mem->values, tail);

  return pair;
}

Val Head(Mem *mem, Val pair)
{
  if (IsNil(pair)) return nil;
  u32 index = RawPair(pair);
  return mem->values[index];
}

Val Tail(Mem *mem, Val pair)
{
  if (IsNil(pair)) return nil;
  u32 index = RawPair(pair);
  return mem->values[index+1];
}

void SetHead(Mem *mem, Val pair, Val val)
{
  Assert(!IsNil(pair));
  u32 index = RawPair(pair);
  mem->values[index] = val;
}

void SetTail(Mem *mem, Val pair, Val val)
{
  Assert(!IsNil(pair));
  u32 index = RawPair(pair);
  mem->values[index+1] = val;
}

Val MakeList(Mem *mem, u32 num, ...)
{
  va_list args;
  va_start(args, num);
  Val list = nil;
  for (u32 i = 0; i < num; i++) {
    Val arg = va_arg(args, Val);
    list = MakePair(mem, arg, list);
  }
  va_end(args);
  return ReverseOnto(mem, list, nil);
}

u32 ListLength(Mem *mem, Val list)
{
  u32 length = 0;
  while (!IsNil(list)) {
    length++;
    list = Tail(mem, list);
  }
  return length;
}

Val ListAt(Mem *mem, Val list, u32 index)
{
  if (IsNil(list)) return nil;
  if (index == 0) return Head(mem, list);
  return ListAt(mem, Tail(mem, list), index - 1);
}

Val ReverseOnto(Mem *mem, Val list, Val tail)
{
  if (IsNil(list)) return tail;
  return ReverseOnto(mem, Tail(mem, list), MakePair(mem, Head(mem, list), tail));
}

Val ListAppend(Mem *mem, Val list1, Val list2)
{
  list1 = ReverseOnto(mem, list1, nil);
  return ReverseOnto(mem, list1, list2);
}

bool IsTagged(Mem *mem, Val list, Val tag)
{
  if (!IsPair(list)) return false;
  return Eq(Head(mem, list), tag);
}

Val MakeTuple(Mem *mem, u32 count)
{
  Val tuple = ObjVal(VecCount(mem->values));
  VecPush(mem->values, TupleHeader(count));

  for (u32 i = 0; i < count; i++) {
    VecPush(mem->values, nil);
  }

  return tuple;
}

u32 TupleLength(Mem *mem, Val tuple)
{
  u32 index = RawObj(tuple);
  return HeaderVal(mem->values[index]);
}

Val TupleAt(Mem *mem, Val tuple, u32 i)
{
  u32 index = RawObj(tuple);
  return mem->values[index + i + 1];
}

void TupleSet(Mem *mem, Val tuple, u32 i, Val val)
{
  u32 index = RawObj(tuple);
  mem->values[index + i + 1] = val;
}

Val MakeSymbolFrom(Mem *mem, char *str, u32 length)
{
  Val sym = SymbolFrom(str, length);
  if (!MapContains(&mem->symbols, sym.as_v)) {
    char *sym_str = Allocate(length + 1);
    for (u32 i = 0; i < length; i++) sym_str[i] = str[i];
    sym_str[length] = '\0';
    MapSet(&mem->symbols, sym.as_v, sym_str);
  }
  return sym;
}

Val MakeSymbol(Mem *mem, char *str)
{
  return MakeSymbolFrom(mem, str, StrLen(str));
}

char *SymbolName(Mem *mem, Val symbol)
{
  return MapGet(&mem->symbols, symbol.as_v);
}

static u32 PrintTail(Mem *mem, Val tail, u32 length)
{
  length += PrintVal(mem, Head(mem, tail));
  if (IsNil(Tail(mem, tail))) {
    Print("]");
    return length + 1;
  }
  if (!IsPair(Tail(mem, tail))) {
    Print(" . ");
    length += 3;
    length += PrintVal(mem, Tail(mem, tail));
    Print("]");
    return length + 1;
  }
  Print(", ");
  return PrintTail(mem, Tail(mem, tail), length + 2);
}

u32 PrintVal(Mem *mem, Val value)
{
  if (IsNil(value)) {
    Print("nil");
    return 3;
  } else if (IsNum(value)) {
    PrintFloat(value.as_f);
    return NumDigits(value.as_f);
  } else if (IsInt(value)) {
    PrintInt(RawInt(value));
    return NumDigits(RawInt(value));
  } else if (IsSym(value)) {
    Print(":");
    char *str = SymbolName(mem, value);
    Print(str);
    return StrLen(str) + 1;
  } else if (IsPair(value)) {
    if (IsTagged(mem, value, SymbolFor("proc"))) {
      Print("[λ");
      Val params = ListAt(mem, value, 1);
      while (!IsNil(params)) {
        Print(" ");
        Print(SymbolName(mem, Head(mem, params)));
        params = Tail(mem, params);
      }
      Print("]");
      return 0;
    } else {
      Print("[");
      return PrintTail(mem, value, 1);
    }
  } else {
    Print("<v");
    PrintIntN(RawObj(value), 4);
    Print(">");
    return 7;
  }
}

void DebugVal(Mem *mem, Val value)
{
  if (IsNil(value)) {
    Print("nil");
  } else if (IsNum(value)) {
    PrintFloat(value.as_f);
  } else if (IsInt(value)) {
    PrintInt(RawInt(value));
  } else if (IsSym(value)) {
    Print(":");
    char *str = SymbolName(mem, value);
    Print(str);
  } else if (IsPair(value)) {
    Print("p");
    PrintInt(RawPair(value));
  } else {
    Print("<v");
    PrintIntN(RawObj(value), 4);
    Print(">");
  }
}

void PrintMem(Mem *mem)
{
  for (u32 i = 0; i < VecCount(mem->values); i++) {
    Print("[");
    PrintIntN(i, 4);
    Print("] ");
    DebugVal(mem, mem->values[i]);
    Print("\n");
  }
}
