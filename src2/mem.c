#include "mem.h"

void InitMem(Mem *mem)
{
  mem->values = NULL;
  VecPush(mem->values, nil);
  VecPush(mem->values, nil);
  mem->strings = NULL;
  InitMap(&mem->string_map);
}

void DestoryMem(Mem *mem)
{
  DestroyMap(&mem->string_map);
  FreeVec(mem->strings);
  FreeVec(mem->values);
}

Val MakeSymbol(char *name, u32 length, Mem *mem)
{
  Val sym = SymbolFrom(name, length);
  u32 index = VecCount(mem->strings);
  for (u32 i = 0; i < length && *name != '\0'; i++) {
    VecPush(mem->strings, *name++);
  }
  VecPush(mem->strings, '\0');
  MapSet(&mem->string_map, sym.as_i, index);
  return sym;
}

char *SymbolName(Val symbol, Mem *mem)
{
  if (!MapContains(&mem->string_map, symbol.as_i)) return NULL;
  u32 index = MapGet(&mem->string_map, symbol.as_i);
  if (index >= VecCount(mem->strings)) return NULL;
  return mem->strings + index;
}

Val Pair(Val head, Val tail, Mem *mem)
{
  u32 index = VecCount(mem->values);
  VecPush(mem->values, head);
  VecPush(mem->values, tail);
  return PairVal(index);
}

Val Head(Val pair, Mem *mem)
{
  Assert(IsPair(pair));
  u32 index = RawVal(pair);
  Assert(index < VecCount(mem->values));
  return mem->values[index];
}

Val Tail(Val pair, Mem *mem)
{
  Assert(IsPair(pair));
  u32 index = RawVal(pair);
  Assert(index < VecCount(mem->values)+1);
  return mem->values[index+1];
}

void PrintVal(Val value, Mem *mem)
{
  if (IsNil(value)) {
    Print("nil");
  } else if (IsNum(value)) {
    PrintFloat(value.as_f, 6);
  } else if (IsInt(value)) {
    PrintInt(RawInt(value));
  } else if (IsSym(value)) {
    Print(":");
    Print(SymbolName(value, mem));
  } else if (IsPair(value)) {
    Print("(");
    PrintVal(Head(value, mem), mem);
    Print(" | ");
    PrintVal(Tail(value, mem), mem);
    Print(")");
  } else {
    Print("<?");
    PrintInt(value.as_i);
    Print(">");
  }
}
