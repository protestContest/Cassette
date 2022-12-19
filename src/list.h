#pragma once
#include "value.h"

Value ListAt(Value list, Value n);
u32 ListLength(Value list);
Value ReverseList(Value list);
Value ReverseListOnto(Value value, Value tail);
void ListPush(Value stack, Value value);
void ListAppend(Value list1, Value list2);
