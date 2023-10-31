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
  Assert(Empty == Sym("empty", symbols));
  Assert(ParseError == Sym("*parse-error*", symbols));
  Assert(Primitive == Sym("*primitive*", symbols));
  Assert(Function == Sym("*function*", symbols));
  Assert(Moved == Sym("*moved*", symbols));
  Assert(Undefined == Sym("*undefined*", symbols));
  Assert(File == Sym("*file*", symbols));
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
