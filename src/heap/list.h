#pragma once
#include "heap.h"

Val Pair(Val head, Val tail, Heap *mem);
Val Head(Val pair, Heap *mem);
Val Tail(Val pair, Heap *mem);
void SetHead(Val pair, Val head, Heap *mem);
void SetTail(Val pair, Val tail, Heap *mem);

bool ListContains(Val list, Val value, Heap *mem);
u32 ListLength(Val list, Heap *mem);
bool IsTagged(Val list, char *tag, Heap *mem);
Val TailList(Val list, u32 pos, Heap *mem);
Val TruncList(Val list, u32 pos, Heap *mem);
Val JoinLists(Val a, Val b, Heap *mem);
Val ListConcat(Val a, Val b, Heap *mem);
Val ListAt(Val list, u32 pos, Heap *mem);
Val ReverseList(Val list, Heap *mem);
