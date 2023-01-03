#pragma once
#include "value.h"

void InitMem(void);
Val GarbageCollect(void);

Val MakePair(Val head, Val tail);
Val Head(Val pair);
Val Tail(Val pair);
void SetHead(Val pair, Val val);
void SetTail(Val pair, Val val);
Val ReverseOnto(Val list, Val tail);
Val Reverse(Val list);

Val MakeTuple(u32 count, ...);
Val TupleLength(Val tuple);
Val TupleAt(Val tuple, u32 i);
void TupleSet(Val tuple, u32 i, Val val);

bool IsTagged(Val exp, Val tag);

Val MakeSymbol(char *src, u32 len);
Val SymbolFor(char *src);
char *SymbolName(Val sym);

Val MakeBinary(char *src, u32 len);
Val BinaryLength(Val binary);
char *BinaryData(Val binary);

void DumpMem(void);
void DumpSymbols(void);
