#pragma once
#include "mem.h"

Val Pair(Val head, Val tail, Mem *mem);
Val Head(Val pair, Mem *mem);
Val Tail(Val pair, Mem *mem);
void SetHead(Val pair, Val head, Mem *mem);
void SetTail(Val pair, Val tail, Mem *mem);

u32 ListLength(Val list, Mem *mem);
bool ListContains(Val list, Val value, Mem *mem);
Val ListAt(Val list, u32 pos, Mem *mem);
Val TailList(Val list, u32 pos, Mem *mem);
Val ListConcat(Val a, Val b, Mem *mem);
Val ReverseList(Val list, Mem *mem);
