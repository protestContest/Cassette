#pragma once
#include "value.h"

typedef struct Mem {
  Val *values;
  Map symbols;
} Mem;

void InitMem(Mem *mem);

Val MakePair(Mem *mem, Val head, Val tail);
Val Head(Mem *mem, Val pair);
Val Tail(Mem *mem, Val pair);
void SetHead(Mem *mem, Val pair, Val val);
void SetTail(Mem *mem, Val pair, Val val);

Val MakeList(Mem *mem, u32 num, ...);
u32 ListLength(Mem *mem, Val list);
Val ListAt(Mem *mem, Val list, u32 index);
Val ReverseOnto(Mem *mem, Val list, Val tail);
Val ListAppend(Mem *mem, Val list1, Val list2);
bool IsTagged(Mem *mem, Val list, Val tag);

Val MakeTuple(Mem *mem, u32 count);
u32 TupleLength(Mem *mem, Val tuple);
Val TupleAt(Mem *mem, Val tuple, u32 i);
void TupleSet(Mem *mem, Val tuple, u32 i, Val val);

Val MakeSymbolFrom(Mem *mem, char *str, u32 length);
Val MakeSymbol(Mem *mem, char *str);
char *SymbolName(Mem *mem, Val symbol);

u32 PrintVal(Mem *mem, Val value);
void PrintMem(Mem *mem);
