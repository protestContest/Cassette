#pragma once
#include "value.h"

typedef struct {
  Val *values;
  char *strings;
  HashMap string_map;
} Mem;

void InitMem(Mem *mem);
void DestroyMem(Mem *mem);
u32 MemSize(Mem *mem);

Val MakeSymbolFrom(char *name, u32 length, Mem *mem);
Val MakeSymbol(char *name, Mem *mem);
char *SymbolName(Val symbol, Mem *mem);

Val Pair(Val head, Val tail, Mem *mem);
Val Head(Val pair, Mem *mem);
Val Tail(Val pair, Mem *mem);
void SetHead(Val pair, Val head, Mem *mem);
void SetTail(Val pair, Val tail, Mem *mem);

bool ListContains(Val list, Val value, Mem *mem);
u32 ListLength(Val list, Mem *mem);
bool IsTagged(Val list, char *tag, Mem *mem);
Val ListConcat(Val a, Val b, Mem *mem);
Val ListFrom(Val list, u32 pos, Mem *mem);
Val ListAt(Val list, u32 pos, Mem *mem);
Val ReverseList(Val list, Mem *mem);
Val ListToBinary(Val list, Mem *mem);

Val MakeBinary(u32 num_bytes, Mem *mem);
Val BinaryFrom(char *text, Mem *mem);
bool IsBinary(Val bin, Mem *mem);
u32 BinaryLength(Val bin, Mem *mem);
char *BinaryData(Val bin, Mem *mem);

Val MakeTuple(u32 length, Mem *mem);
bool IsTuple(Val tuple, Mem *mem);
u32 TupleLength(Val tuple, Mem *mem);
bool TupleContains(Val tuple, Val value, Mem *mem);
void TupleSet(Val tuple, u32 index, Val value, Mem *mem);
Val TuplePut(Val tuple, u32 index, Val value, Mem *mem);
Val TupleGet(Val tuple, u32 index, Mem *mem);
u32 TupleIndexOf(Val tuple, Val value, Mem *mem);

Val MakeMap(u32 capacity, Mem *mem);
Val MapFrom(Val keys, Val values, Mem *mem);
bool IsMap(Val map, Mem *mem);
void MapSet(Val map, Val key, Val value, Mem *mem);
Val MapPut(Val map, Val key, Val value, Mem *mem);
Val MapPutPath(Val map, Val path, Val value, Mem *mem);
Val MapGet(Val map, Val key, Mem *mem);
Val MapDelete(Val map, Val key, Mem *mem);
bool MapContains(Val map, Val key, Mem *mem);
u32 MapCount(Val map, Mem *mem);
Val MapKeys(Val map, Mem *mem);
Val MapValues(Val map, Mem *mem);

Val MakeFunction(u32 entry, u32 arity, Val env, Mem *mem);
Val MakePrimitive(u32 num, Mem *mem);
bool IsFunction(Val func, Mem *mem);
bool IsPrimitive(Val func, Mem *mem);
u32 FunctionEntry(Val func, Mem *mem);
u32 FunctionArity(Val func, Mem *mem);
Val FunctionEnv(Val func, Mem  *mem);

bool PrintVal(Val value, Mem *mem);
u32 InspectVal(Val value, Mem *mem);
void PrintMem(Mem *mem);
void PrintSymbols(Mem *mem);

void CollectGarbage(Val *roots, Mem *mem);
