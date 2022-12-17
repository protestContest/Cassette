#pragma once

typedef struct VM VM;

typedef u32 Value;
typedef u32 ObjHeader;

void InitVM(void);

Value MakePair(Value head, Value tail);
Value Head(Value pair);
Value Tail(Value pair);
void SetHead(Value pair, Value head);
void SetTail(Value pair, Value tail);

ObjHeader *HeapRef(Value index);
Value Allocate(ObjHeader header, u32 bytes);

void DumpPairs(void);
void DumpHeap(void);

Value CreateSymbol(char *src, u32 start, u32 end);
u32 LongestSymLength(void);
Value SymbolName(Value sym);
void DumpSymbols(void);
