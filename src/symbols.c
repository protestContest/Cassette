#include "symbols.h"
#include "mem.h"
#include <stdlib.h>
#include <string.h>

void InitSymbolTable(SymbolTable *symbols)
{
  symbols->count = 0;
  symbols->capacity = 256;
  symbols->names = malloc(256);
  InitHashMap(&symbols->map);

  Assert(True == Sym("true", symbols));
  Assert(False == Sym("false", symbols));
  Assert(Ok == Sym("ok", symbols));
  Assert(Error == Sym("error", symbols));

#ifdef DEBUG
  Assert(Primitive == Sym("*prim*", symbols));
  Assert(Undefined == Sym("*undefined*", symbols));
  Assert(File == Sym("*file*", symbols));
#endif
}

void DestroySymbolTable(SymbolTable *symbols)
{
  symbols->count = 0;
  symbols->capacity = 0;
  free(symbols->names);
  DestroyHashMap(&symbols->map);
}

Val SymbolFor(char *name)
{
  return SymVal(Hash(name, strlen(name)));
}

Val SymbolFrom(char *name, u32 length)
{
  return SymVal(Hash(name, length));
}

Val Sym(char *name, SymbolTable *symbols)
{
  return MakeSymbol(name, strlen(name), symbols);
}

static void AddSymbol(char *name, u32 length, SymbolTable *symbols)
{
  if (symbols->count >= symbols->capacity) {
    u32 needed = Max(symbols->capacity*2, symbols->capacity+length);
    symbols->names = realloc(symbols->names, needed);
  }
  memcpy(symbols->names + symbols->count, name, length);
  symbols->count += length;
  symbols->names[symbols->count++] = 0;
}

Val MakeSymbol(char *name, u32 length, SymbolTable *symbols)
{
  Val symbol = SymVal(Hash(name, length));
  if (!HashMapContains(&symbols->map, symbol)) {
    HashMapSet(&symbols->map, symbol, symbols->count);
    AddSymbol(name, length, symbols);
  }
  return symbol;
}

char *SymbolName(Val sym, SymbolTable *symbols)
{
  if (HashMapContains(&symbols->map, sym)) {
    return symbols->names + HashMapGet(&symbols->map, sym);
  } else {
    return 0;
  }
}

u32 LongestSymbol(SymbolTable *symbols)
{
  char *cur = symbols->names;
  u32 max_length = 0;
  while (cur < symbols->names + symbols->count) {
    u32 length = StrLen(cur);
    max_length = Max(max_length, length);
    cur += length + 1;
  }
  return max_length;
}
