#pragma once
#include "mem.h"
#include "value.h"

extern Value nil_val;
extern Value true_val;
extern Value false_val;

void InitSymbols(void);
void ResetSymbols(void);

Value CreateSymbol(char *src, u32 start, u32 end);

u32 LongestSymLength(void);
void DumpSymbols(void);
Value SymbolName(Value sym);
