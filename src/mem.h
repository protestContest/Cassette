#pragma once
#include "value.h"
#include "symbol.h"

void InitMem(void);

Value MakePair(Value head, Value tail);
Value Head(Value obj);
Value Tail(Value obj);
void SetHead(Value obj, Value head);
void SetTail(Value obj, Value tail);

Value MakeVec(u32 count);
Value VecGet(Value vec, u32 n);
void VecSet(Value vec, u32 n, Value val);

u32 Allocate(u32 size);
u32 AllocateObject(ObjHeader header, u32 size);
ObjHeader *ObjectRef(Value value);

void DumpPairs(void);
