#include "mem.h"
#include "univ.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Val FloatVal(float num)
{
  union {Val as_v; float as_f;} convert;
  convert.as_f = num;
  return convert.as_v;
}

float RawFloat(Val value)
{
  union {Val as_v; float as_f;} convert;
  convert.as_v = value;
  return convert.as_f;
}

void PrintVal(Val value)
{
  if (IsInt(value)) {
    printf("%u", RawInt(value));
  } else if (IsFloat(value)) {
    printf("%f", RawFloat(value));
  } else {
    printf("%08X", value);
  }
}

void InitMem(Mem *mem, u32 size)
{
  mem->values = (Val**)NewHandle(sizeof(Val) * size);
  mem->count = 0;
  mem->capacity = size;
  InitSymbolTable(&mem->symbols);
}

void DestroyMem(Mem *mem)
{
  DisposeHandle((Handle)mem->values);
  mem->count = 0;
  mem->capacity = 0;
  mem->values = NULL;
  DestroySymbolTable(&mem->symbols);
}

void InitSymbolTable(SymbolTable *symbols)
{
  symbols->count = 0;
  symbols->capacity = 256;
  symbols->names = malloc(256);
  InitHashMap(&symbols->map);
}

void DestroySymbolTable(SymbolTable *symbols)
{
  symbols->count = 0;
  symbols->capacity = 0;
  free(symbols->names);
  DestroyHashMap(&symbols->map);
}

Val SymbolFor(char *name)
{
  return SymVal(Hash(name, strlen(name)));
}

Val Sym(char *name, Mem *mem)
{
  return MakeSymbol(name, strlen(name), &mem->symbols);
}

static void AddSymbol(char *name, u32 length, SymbolTable *symbols)
{
  if (symbols->count >= symbols->capacity) {
    u32 needed = Max(symbols->capacity*2, symbols->capacity+length);
    symbols->names = realloc(symbols->names, needed);
  }
  memcpy(symbols->names + symbols->count, name, length);
  symbols->count += length;
  symbols->names[symbols->count++] = 0;
}

Val MakeSymbol(char *name, u32 length, SymbolTable *symbols)
{
  Val symbol = SymVal(Hash(name, length));
  if (!HashMapContains(&symbols->map, symbol)) {
    HashMapSet(&symbols->map, symbol, symbols->count);
    AddSymbol(name, length, symbols);
  }
  return symbol;
}

char *SymbolName(Val sym, Mem *mem)
{
  if (HashMapContains(&mem->symbols.map, sym)) {
    return mem->symbols.names + HashMapGet(&mem->symbols.map, sym);
  } else {
    return 0;
  }
}

Val Pair(Val head, Val tail, Mem *mem)
{
  Val pair = PairVal(mem->count);
  Assert(mem->count + 2 <= mem->capacity);
  (*mem->values)[mem->count++] = head;
  (*mem->values)[mem->count++] = tail;
  return pair;
}

Val Head(Val pair, Mem *mem)
{
  Assert(IsPair(pair));
  return (*mem->values)[RawVal(pair)];
}

Val Tail(Val pair, Mem *mem)
{
  Assert(IsPair(pair));
  return (*mem->values)[RawVal(pair)+1];
}

Val MakeTuple(u32 length, Mem *mem)
{
  u32 i;
  Val tuple = ObjVal(mem->count);
  Assert(mem->count + length + 1 <= mem->capacity);
  (*mem->values)[mem->count++] = TupleHeader(length);
  for (i = 0; i < length; i++) {
    (*mem->values)[mem->count++] = Nil;
  }
  return tuple;
}

bool IsTuple(Val value, Mem *mem)
{
  return IsObj(value) && IsTupleHeader((*mem->values)[RawVal(value)]);
}

u32 TupleLength(Val tuple, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  return RawVal((*mem->values)[RawVal(tuple)]);
}

void TupleSet(Val tuple, u32 i, Val value, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  Assert(i < TupleLength(tuple, mem));
  (*mem->values)[RawVal(tuple) + i + 1] = value;
}

Val TupleGet(Val tuple, u32 i, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  Assert(i < TupleLength(tuple, mem));
  return (*mem->values)[RawVal(tuple) + i + 1];
}

#define NumBinCells(length)   ((length + 1) / 4 - 1)

Val MakeBinary(u32 length, Mem *mem)
{
  Val binary = ObjVal(mem->count);
  u32 cells = NumBinCells(length);
  u32 i;
  Assert(mem->count + cells + 1 <= mem->capacity);
  (*mem->values)[mem->count++] = BinaryHeader(length);
  for (i = 0; i < cells; i++) {
    (*mem->values)[mem->count++] = 0;
  }
  return binary;
}

bool IsBinary(Val value, Mem *mem)
{
  return IsObj(value) && IsBinaryHeader((*mem->values)[RawVal(value)]);
}

u32 BinaryLength(Val binary, Mem *mem)
{
  Assert(IsBinary(binary, mem));
  return RawVal((*mem->values)[RawVal(binary)]);
}

void *BinaryData(Val binary, Mem *mem)
{
  Assert(IsBinary(binary, mem));
  return &(*mem->values)[RawVal(binary)];
}
