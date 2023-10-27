#pragma once
#include "univ.h"
#include "mem.h"

typedef struct SymbolTable {
  u32 count;
  u32 capacity;
  char *names;
  HashMap map;
} SymbolTable;

void InitSymbolTable(SymbolTable *symbols);
void DestroySymbolTable(SymbolTable *symbols);
Val SymbolFor(char *name);
Val Sym(char *name, SymbolTable *symbols);
Val MakeSymbol(char *name, u32 length, SymbolTable *symbols);
char *SymbolName(Val sym, SymbolTable *symbols);
