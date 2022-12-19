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
Value Allocate(ObjHeader header);

void DumpPairs(void);
void DumpHeap(void);

Value MakeSymbolFrom(Value text, Value start, Value end);
Value CreateSymbol(char *src);
Value GetSymbol(char *src);
u32 LongestSymLength(void);
Value SymbolName(Value sym);
void DumpSymbols(void);
