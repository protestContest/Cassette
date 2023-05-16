#pragma once
#include "value.h"

typedef struct {
  Val *values;
  Map symbols;
  char **symbol_names;
} Mem;

void InitMem(Mem *mem, u32 size);
void DestroyMem(Mem *mem);

Val MakeSymbolFrom(Mem *mem, char *str, u32 length);
Val MakeSymbol(Mem *mem, char *str);
char *SymbolName(Mem *mem, Val symbol);

Val MakePair(Mem *mem, Val head, Val tail);
Val Head(Mem *mem, Val pair);
Val Tail(Mem *mem, Val pair);
void SetHead(Mem *mem, Val pair, Val val);
void SetTail(Mem *mem, Val pair, Val val);

u32 ListLength(Mem *mem, Val list);
Val ListAt(Mem *mem, Val list, u32 index);
Val ListFrom(Mem *mem, Val list, u32 index);
Val ReverseOnto(Mem *mem, Val list, Val tail);
Val ListAppend(Mem *mem, Val list, Val value);
Val ListConcat(Mem *mem, Val list1, Val list2);
bool IsTagged(Mem *mem, Val list, char *tag);
#define First(mem, list)  ListAt(mem, list, 0)
#define Second(mem, list)  ListAt(mem, list, 1)
#define Third(mem, list)  ListAt(mem, list, 2)
#define Fourth(mem, list)  ListAt(mem, list, 3)

bool IsTuple(Mem *mem, Val tuple);
Val MakeTuple(Mem *mem, u32 count);
u32 TupleLength(Mem *mem, Val tuple);
Val TupleAt(Mem *mem, Val tuple, u32 i);
void TupleSet(Mem *mem, Val tuple, u32 i, Val val);

bool IsMap(Mem *mem, Val value);

bool IsBinary(Mem *mem, Val binary);
Val MakeBinaryFrom(Mem *mem, char *str, u32 length);
Val MakeBinary(Mem *mem, u32 size);
u32 BinaryLength(Mem *mem, Val binary);
u8 *BinaryData(Mem *mem, Val binary);
void BinaryToString(Mem *mem, Val binary, char *dst);

u32 ValToString(Mem *mem, Val val, Buf *buf);
u32 PrintValStr(Mem *mem, Val val);
u32 ValStrLen(Mem *mem, Val val);
