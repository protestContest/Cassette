#include "symbol.h"
#include "binary.h"
#include "tuple.h"
#include "map.h"
#include "list.h"
#include "univ/string.h"
#include "univ/hash.h"
#include "univ/memory.h"

Val MakeSymbol(char *name, Heap *mem)
{
  return MakeSymbolFrom(name, StrLen(name), mem);
}

Val MakeSymbolFrom(char *name, u32 length, Heap *mem)
{
  Val sym = SymbolFrom(name, length);
  if (HashMapContains(&mem->string_map, sym.as_i)) {
    return sym;
  }

  u32 bytes_needed = mem->strings_count + length + 1;
  if (bytes_needed < mem->strings_capacity) {
    mem->strings_capacity += bytes_needed;
    mem->strings = Reallocate(mem->strings, mem->strings_capacity);
  }

  u32 index = mem->strings_count;
  Copy(name, mem->strings + index, length);
  mem->strings[index + length] = '\0';
  HashMapSet(&mem->string_map, sym.as_i, index);

  return sym;
}

char *SymbolName(Val symbol, Heap *mem)
{
  if (!HashMapContains(&mem->string_map, symbol.as_i)) return NULL;

  u32 index = HashMapGet(&mem->string_map, symbol.as_i);
  return mem->strings + index;
}

Val HashValue(Val value, Heap *mem)
{
  if (IsSym(value)) return value;
  if (IsFloat(value)) return SymVal(Hash(&value, sizeof(value)));
  if (IsInt(value)) {
    u32 n = RawInt(value);
    return SymVal(Hash(&n, sizeof(n)));
  }
  if (IsBinary(value, mem)) {
    char *data = BinaryData(value, mem);
    return SymVal(Hash(data, BinaryLength(value, mem)));
  }
  if (IsNil(value)) {
    return SymVal(EmptyHash);
  }
  if (IsPair(value)) {
    u32 hash = EmptyHash ^ pairMask;
    Val head = HashValue(Head(value, mem), mem);
    Val tail = HashValue(Tail(value, mem), mem);
    return SymVal(hash ^ RawVal(head) ^ RawVal(tail));
  }
  if (IsTuple(value, mem)) {
    u32 hash = EmptyHash ^ tupleMask;
    for (u32 i = 0; i < TupleLength(value, mem); i++) {
      Val item_hash = HashValue(TupleGet(value, i, mem), mem);
      hash = hash ^ RawVal(item_hash);
    }
    return SymVal(hash);
  }
  if (IsMap(value, mem)) {
    Val keys = HashValue(MapKeys(value, mem), mem);
    Val values = HashValue(MapValues(value, mem), mem);
    u32 hash = EmptyHash ^ mapMask;
    return SymVal(hash ^ RawVal(keys) ^ RawVal(values));
  }

  return SymVal(Hash(&value, sizeof(value)));
}
