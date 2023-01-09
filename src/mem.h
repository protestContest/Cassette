#pragma once
#include "value.h"

void InitMem(void);

Val MakePair(Val head, Val tail);
Val Head(Val pair);
Val Tail(Val pair);
void SetHead(Val pair, Val val);
void SetTail(Val pair, Val val);
Val MakeList(u32 length, ...);
Val ReverseOnto(Val list, Val tail);
Val Reverse(Val list);
u32 ListLength(Val list);

Val MakeTuple(u32 count, ...);
Val ListToTuple(Val list);
u32 TupleLength(Val tuple);
Val TupleAt(Val tuple, u32 i);
void TupleSet(Val tuple, u32 i, Val val);

bool IsTagged(Val exp, Val tag);

Val MakeSymbol(char *src, u32 len);
Val SymbolFor(char *src);
char *SymbolName(Val sym);
void DumpSymbols(void);

Val MakeBinary(char *src, u32 len);
u32 BinaryLength(Val binary);
char *BinaryData(Val binary);
char *BinToCStr(Val binary);

Val MakeDict(Val pairs);
u32 DictSize(Val dict);
Val DictGet(Val dict, Val key);
Val DictKeyAt(Val dict, u32 i);
Val DictValueAt(Val dict, u32 i);
