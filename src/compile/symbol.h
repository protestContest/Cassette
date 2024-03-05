#pragma once

void InitSymbols(void);
u32 AddSymbol(char *name);
u32 AddSymbolLen(char *name, u32 len);
char *SymbolName(u32 sym);
