#include "symbols.h"
#include "univ/hash.h"
#include "univ/math.h"
#include "univ/system.h"
#include "univ/str.h"

#include "debug.h"

void InitSymbolTable(SymbolTable *symbols)
{
  InitVec((Vec*)&symbols->names, sizeof(char), 256);
  InitHashMap(&symbols->map);
}

void DestroySymbolTable(SymbolTable *symbols)
{
  DestroyVec((Vec*)&symbols->names);
  DestroyHashMap(&symbols->map);
}

Val SymbolFor(char *name)
{
  return SymbolFrom(name, StrLen(name));
}

Val SymbolFrom(char *name, u32 length)
{
  return SymVal(FoldHash(Hash(name, length), valBits));
}

Val Sym(char *name, SymbolTable *symbols)
{
  return MakeSymbol(name, StrLen(name), symbols);
}

static void AddSymbol(char *name, u32 length, SymbolTable *symbols)
{
  u32 index = GrowVec((Vec*)&symbols->names, sizeof(*symbols->names.items), length+1);
  Copy(name, symbols->names.items + index, length);
  symbols->names.items[symbols->names.count-1] = 0;
}

Val MakeSymbol(char *name, u32 length, SymbolTable *symbols)
{
  Val symbol = SymbolFrom(name, length);
  if (!HashMapContains(&symbols->map, symbol)) {
    HashMapSet(&symbols->map, symbol, symbols->names.count);
    AddSymbol(name, length, symbols);
  }
  return symbol;
}

char *SymbolName(Val sym, SymbolTable *symbols)
{
  if (HashMapContains(&symbols->map, sym)) {
    return (char*)symbols->names.items + HashMapGet(&symbols->map, sym);
  } else {
    return "?";
  }
}

u32 LongestSymbol(SymbolTable *symbols)
{
  u8 *cur = symbols->names.items;
  u32 max_length = 0;
  while (cur < symbols->names.items + symbols->names.count) {
    u32 length = StrLen((char*)cur);
    max_length = Max(max_length, length);
    cur += length + 1;
  }
  return max_length;
}

void DeserializeSymbols(u32 count, char *names, SymbolTable *symbols)
{
  char *cur = names;
  symbols->names.count = count;
  symbols->names.capacity = count;
  symbols->names.items = Alloc(count);
  Copy(names, symbols->names.items, count);

  InitHashMap(&symbols->map);
  while (cur < names + count) {
    u32 len = StrLen(cur);
    Val sym = SymbolFrom(cur, len);
    HashMapSet(&symbols->map, sym, cur - names);
    cur += len + 1;
  }
}
