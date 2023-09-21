#pragma once
#include "value.h"
#include "univ.h"

struct Mem;

typedef struct {
  u32 capacity;
  u32 count;
  char **names;
  HashMap map;
} SymbolMap;

void InitSymbolMap(SymbolMap *symbols);
void DestroySymbolMap(SymbolMap *symbols);
Val MakeSymbol(char *name, struct Mem *mem);
Val MakeSymbolFrom(char *name, u32 length, struct Mem *mem);
char *SymbolName(Val symbol, struct Mem *mem);
