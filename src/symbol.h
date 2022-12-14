#pragma once
#include "mem.h"

void InitSymbols(void);
Value CreateSymbol(char *src, u32 len);

u32 LongestSymLength(void);
void DumpSymbols(void);
char *SymbolName(Value sym);
