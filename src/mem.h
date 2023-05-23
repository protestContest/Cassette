#pragma once
#include "value.h"
#include "string_table.h"

typedef struct {
  Val *values;
  StringTable symbols;
} Mem;

void InitMem(Mem *mem, u32 size);
void DestroyMem(Mem *mem);

#define MakeSymbolFrom(mem, str, length)  PutString(&(mem)->symbols, str, length)
#define MakeSymbol(mem, str)              PutString(&(mem)->symbols, str, StrLen(str))
#define SymbolName(mem, symbol)           GetString(&(mem)->symbols, symbol)
#define SymbolLength(mem, symbol)         GetStringLength(&(mem)->symbols, symbol)
#define PrintSymbol(mem, symbol)          PrintN(SymbolName(mem, symbol), SymbolLength(mem, symbol))

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
#define Fifth(mem, list)  ListAt(mem, list, 4)
#define Sixth(mem, list)  ListAt(mem, list, 5)

bool IsTuple(Mem *mem, Val tuple);
Val MakeTuple(Mem *mem, u32 count);
Val TupleFromList(Mem *mem, Val list);
u32 TupleLength(Mem *mem, Val tuple);
Val TupleAt(Mem *mem, Val tuple, u32 i);
void TupleSet(Mem *mem, Val tuple, u32 i, Val val);

bool IsMap(Mem *mem, Val map);
Val MakeMap(Mem *mem, Val items);
u32 MapSize(Mem *mem, Val map);
Val MapKeys(Mem *mem, Val map);
Val MapVals(Mem *mem, Val map);

bool IsBinary(Mem *mem, Val binary);
u32 NumBinaryCells(u32 length);
Val MakeBinaryFrom(Mem *mem, char *str, u32 length);
Val MakeBinary(Mem *mem, u32 size);
u32 BinaryLength(Mem *mem, Val binary);
u8 *BinaryData(Mem *mem, Val binary);
void BinaryToString(Mem *mem, Val binary, char *dst);

u32 ValToString(Mem *mem, Val val, Buf *buf);
u32 PrintValStr(Mem *mem, Val val);
u32 ValStrLen(Mem *mem, Val val);

void PrintMem(Mem *mem);
