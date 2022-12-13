#pragma once
#include "mem.h"

void InitSymbols(void);
Value CreateSymbol(char *src, u32 len);
void PrintSymbol(Value sym, u32 len);
void PrintSymbols(void);
u32 LongestSymLength(void);
