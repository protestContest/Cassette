#pragma once
#include "../value.h"

typedef struct {
  Val *mem;
  Val symbols;
} Heap;

void InitMem(Val **mem);
void InitHeap(Heap *heap);

Val MakePair(Val **mem, Val head, Val tail);
Val Head(Val *mem, Val pair);
Val Tail(Val *mem, Val pair);
void SetHead(Val **mem, Val pair, Val val);
void SetTail(Val **mem, Val pair, Val val);

Val MakeList(Val **mem, u32 length, ...);
Val ReverseOnto(Val **mem, Val list, Val tail);
Val Reverse(Val **mem, Val list);
u32 ListLength(Val *mem, Val list);
Val ListAt(Val *mem, Val list, u32 index);
Val First(Val *mem, Val list);
Val Second(Val *mem, Val list);
Val Third(Val *mem, Val list);
Val ListLast(Val *mem, Val list);
void ListAppend(Val **mem, Val list1, Val list2);

Val MakeTuple(Val **mem, u32 count);
u32 TupleLength(Val *mem, Val tuple);
Val TupleAt(Val *mem, Val tuple, u32 i);
void TupleSet(Val *mem, Val tuple, u32 i, Val val);

Val MakeBinary(Val **mem, char *src, u32 len);
u32 BinaryLength(Val *mem, Val bin);
u8 *BinaryData(Val *mem, Val bin);

Val SymbolFor(char *src);
Val SymbolFrom(char *src, u32 length);
Val BinToSym(Val *mem, Val bin);

Val MakeMap(Val **mem, u32 count);
void MapPut(Val *mem, Val map, Val key, Val val);
bool MapHasKey(Val *mem, Val map, Val key);
Val MapGet(Val *mem, Val map, Val key);
u32 MapSize(Val *mem, Val map);
Val MapKeyAt(Val *mem, Val map, u32 i);
Val MapValAt(Val *mem, Val map, u32 i);

void PrintTree(Val *mem, Val symbols, Val root);
