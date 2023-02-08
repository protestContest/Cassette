#pragma once
#include "value.h"

typedef struct {
  Val key;
  char *name;
} Symbol;

void InitMem(Val *mem);

Val MakePair(Val **mem, Val head, Val tail);
Val Head(Val *mem, Val pair);
Val Tail(Val *mem, Val pair);
void SetHead(Val **mem, Val pair, Val val);
void SetTail(Val **mem, Val pair, Val val);

Val MakeList(Val **mem, u32 length, ...);
// Val MakeTagged(Val *mem, u32 length, char *name, ...);
// bool IsTagged(Val *mem, Val exp, char *tag);
Val ReverseOnto(Val **mem, Val list, Val tail);
Val Reverse(Val **mem, Val list);
// Val Flatten(Val *mem, Val list);
u32 ListLength(Val *mem, Val list);
Val ListAt(Val *mem, Val list, u32 index);
Val First(Val *mem, Val list);
Val Second(Val *mem, Val list);
Val Third(Val *mem, Val list);
Val ListLast(Val *mem, Val list);
void ListAppend(Val **mem, Val list1, Val list2);

Val MakeTuple(Val **mem, u32 count, ...);
// Val ListToTuple(Val *mem, Val list);
u32 TupleLength(Val *mem, Val tuple);
Val TupleAt(Val *mem, Val tuple, u32 i);
void TupleSet(Val *mem, Val tuple, u32 i, Val val);

Val MakeSymbol(Symbol **symbols, char *src);
Val MakeSymbolFromSlice(Symbol **symbols, char *src, u32 len);
// Val BoolSymbol(bool val);
Val SymbolFor(char *src);
char *SymbolName(Symbol *symbols, Val sym);
void DumpSymbols(Symbol *symbols);

Val MakeBinary(Val *mem, char *src, u32 len);
u32 BinaryLength(Val *mem, Val binary);
char *BinaryData(Val *mem, Val binary);
char *BinToCStr(Val *mem, Val binary);
Val BinaryAt(Val *mem, Val binary, u32 i);

void PrintHeap(Val *mem, Symbol *symbols);

Val MakeDict(Val **mem, u32 count);
void DictPut(Val *mem, Val dict, Val key, Val val);
bool DictHasKey(Val *mem, Val dict, Val key);
Val DictGet(Val *mem, Val dict, Val key);
u32 DictSize(Val *mem, Val dict);
Val DictKeyAt(Val *mem, Val dict, u32 i);
Val DictValAt(Val *mem, Val dict, u32 i);
