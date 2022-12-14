#pragma once
#include "value.h"

typedef struct {
  Value head;
  Value tail;
} Pair;

void InitMem(void);

Value MakePair(Value head, Value tail);
Value Head(Value obj);
Value Tail(Value obj);
void SetHead(Value obj, Value head);
void SetTail(Value obj, Value tail);

// Value MakeVec(u32 count);
// Value VecGet(Value vec, u32 n);
// void VecSet(Value vec, u32 n, Value val);

void DumpPairs(void);
