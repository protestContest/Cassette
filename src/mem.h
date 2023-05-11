#pragma once
#include "value.h"

typedef struct Mem {
  Val *values;
  Map symbols;
  char **symbol_names;
} Mem;

void InitMem(Mem *mem, u32 size);
void DestroyMem(Mem *mem);

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

bool IsDict(Mem *mem, Val dict);
Val MakeDict(Mem *mem);
Val DictFrom(Mem *mem, Val keys, Val vals);
u32 DictSize(Mem *mem, Val dict);
Val DictKeys(Mem *mem, Val dict);
Val DictValues(Mem *mem, Val dict);
bool InDict(Mem *mem, Val dict, Val key);
Val DictGet(Mem *mem, Val dict, Val key);
Val DictSet(Mem *mem, Val dict, Val key, Val value);

Val MakeSymbolFrom(Mem *mem, char *str, u32 length);
Val MakeSymbol(Mem *mem, char *str);
char *SymbolName(Mem *mem, Val symbol);

bool IsBinary(Mem *mem, Val binary);
Val MakeBinaryFrom(Mem *mem, char *str, u32 length);
Val MakeBinary(Mem *mem, char *str);
u32 BinaryLength(Mem *mem, Val binary);
u8 *BinaryData(Mem *mem, Val binary);
void BinaryToString(Mem *mem, Val binary, char *dst);

u32 ValToString(Mem *mem, Val val, Buf *buf);
u32 PrintValStr(Mem *mem, Val val);
u32 ValStrLen(Mem *mem, Val val);

u32 PrintVal(Mem *mem, Val value);
void DebugVal(Mem *mem, Val value);
void PrintMem(Mem *mem);
