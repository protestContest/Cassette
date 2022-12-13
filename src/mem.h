#pragma once
#include "value.h"

typedef struct {
  Value head;
  Value tail;
} Pair;

void InitMem(u32 num_pairs);
void PrintMem(void);

Value MakePair(Value head, Value tail);
Value Head(Value obj);
Value Tail(Value obj);
void SetHead(Value obj, Value head);
void SetTail(Value obj, Value tail);
