#pragma once
#include "value.h"

u32 ListLength(Value list);
Value ReverseList(Value list);
Value ReverseListOnto(Value value, Value tail);
void ListPush(Value stack, Value value);
