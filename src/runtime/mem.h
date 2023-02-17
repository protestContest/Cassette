#pragma once
#include "../value.h"

typedef struct StringMap StringMap;

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

Val MakeTuple(Val **mem, u32 count);
u32 TupleLength(Val *mem, Val tuple);
Val TupleAt(Val *mem, Val tuple, u32 i);
void TupleSet(Val *mem, Val tuple, u32 i, Val val);

void PrintHeap(Val *mem, StringMap *strings);

Val MakeMap(Val **mem, u32 count);
void MapPut(Val *mem, Val map, Val key, Val val);
bool MapHasKey(Val *mem, Val map, Val key);
Val MapGet(Val *mem, Val map, Val key);
u32 MapSize(Val *mem, Val map);
Val MapKeyAt(Val *mem, Val map, u32 i);
Val MapValAt(Val *mem, Val map, u32 i);
