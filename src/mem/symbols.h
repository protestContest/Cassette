#pragma once
#include "mem.h"
#include "univ/hashmap.h"
#include "univ/vec.h"

/* pre-computed symbols */
#define True              0x7FD7E33F /* true */
#define False             0x7FDAF4C4 /* false */
#define Ok                0x7FDDDE52 /* ok */
#define Error             0x7FD04FC6 /* error */
#define Primitive         0x7FD15938 /* *prim* */
#define Function          0x7FD4D0E4 /* *func* */
#define Undefined         0x7FDBC789 /* *undefined* */
#define Moved             0x7FDE7580 /* *moved* */
#define File              0x7FDA0003 /* *file* */
#define FloatType         0x7FDF2D07 /* float */
#define NumType           0x7FDFEE5C /* number */

/*
Symbol names are kept in a dynamic array of characters, where each name is a
null-terminated C string. A hash map maps a symbol's value to the starting
offset in the name array.

Symbols do not need to exist in a symbol table to be useful. The symbol table is
only necessary to get the name of a symbol, e.g. to print it.

SymbolFor and SymbolFrom create symbol values without storing their names in a
table. Sym and MakeSymbol are the equivalents that do save the name. SymbolName
retrieves the name of a symbol from a table, if it exists. LongestSymbol gets
the length of the longest name, which is sometimes useful for printing.
*/

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
