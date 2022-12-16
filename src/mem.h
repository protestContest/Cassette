#pragma once
#include "value.h"
#include "symbol.h"

#define ObjectAddr(v)   (RawVal(v)*4)

void InitMem(void);

Value MakePair(Value head, Value tail);
Value Head(Value pair);
Value Tail(Value pair);
void SetHead(Value pair, Value head);
void SetTail(Value pair, Value tail);

Value MakeVec(u32 count);
Value VecGet(Value vec, u32 n);
void VecSet(Value vec, u32 n, Value val);

u32 Allocate(u32 size);
u32 AllocateObject(ObjHeader header, u32 size);
ObjHeader *ObjectRef(Value value);

void DumpPairs(void);
void DumpHeap(void);
void HexDump(u8 *data, u32 size, char *title);
