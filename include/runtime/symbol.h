#pragma once

/*
 * A symbol is a hash of the symbol name. Names are stored in a static block and can be retrieved
 * later.
 */

u32 Symbol(char *name);
u32 SymbolFrom(char *name, u32 len);
char *SymbolName(u32 sym);
void SetSymbolSize(i32 size);
