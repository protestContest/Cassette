#pragma once
#include "mem.h"
#include "univ/hashmap.h"
#include "univ/vec.h"

typedef struct SymbolTable {
  ByteVec names;
  HashMap map;
} SymbolTable;

void InitSymbolTable(SymbolTable *symbols);
void DestroySymbolTable(SymbolTable *symbols);
Val SymbolFor(char *name);
Val SymbolFrom(char *name, u32 length);
Val Sym(char *name, SymbolTable *symbols);
Val MakeSymbol(char *name, u32 length, SymbolTable *symbols);
char *SymbolName(Val sym, SymbolTable *symbols);
u32 LongestSymbol(SymbolTable *symbols);
