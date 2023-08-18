#pragma once
#include "heap.h"

Val MakeSymbol(char *name, Heap *mem);
Val MakeSymbolFrom(char *name, u32 length, Heap *mem);
char *SymbolName(Val symbol, Heap *mem);
