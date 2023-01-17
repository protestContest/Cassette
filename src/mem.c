#include "mem.h"
#include "printer.h"
#include "hash.h"
#include "env.h"

#define MEM_SIZE      (4096*4096)
Val mem[MEM_SIZE];
u32 mem_next = 2;

typedef struct {
  Val key;
  char *name;
} Symbol;

#define NUM_SYMBOLS 256
Symbol symbols[NUM_SYMBOLS];
u32 sym_next = 0;

Val nil = PairVal(0);

Val MakePair(Val head, Val tail)
{
  if (mem_next+1 >= MEM_SIZE) Error("Out of memory");

  Val pair = PairVal(mem_next);

  mem[mem_next++] = head;
  mem[mem_next++] = tail;

  return pair;
}

Val Head(Val pair)
{
  if (IsNil(pair)) return nil;
  u32 index = RawVal(pair);
  return mem[index];
}

Val Tail(Val pair)
{
  if (IsNil(pair)) return nil;
  u32 index = RawVal(pair);
  return mem[index+1];
}

void SetHead(Val pair, Val val)
{
  if (IsNil(pair)) Error("Can't change nil");
  u32 index = RawVal(pair);
  mem[index] = val;
}

void SetTail(Val pair, Val val)
{
  if (IsNil(pair)) Error("Can't change nil");
  u32 index = RawVal(pair);
  mem[index+1] = val;
}

Val MakeList(u32 length, ...)
{
  Val list = nil;
  va_list args;
  va_start(args, length);
  for (u32 i = 0; i < length; i++) {
    Val arg = va_arg(args, Val);
    list = MakePair(arg, list);
  }
  va_end(args);
  return Reverse(list);
}

Val MakeTagged(u32 length, char *name, ...)
{
  Val list = MakePair(MakeSymbol(name), nil);
  va_list args;
  va_start(args, name);
  for (u32 i = 0; i < length - 1; i++) {
    Val arg = va_arg(args, Val);
    list = MakePair(arg, list);
  }
  va_end(args);
  return Reverse(list);
}

Val ReverseOnto(Val list, Val tail)
{
  if (IsNil(list)) return tail;

  Val rest = Tail(list);
  SetTail(list, tail);
  return ReverseOnto(rest, list);
}

Val Reverse(Val list)
{
  return ReverseOnto(list, nil);
}

Val Flatten(Val list)
{
  Val result = nil;
  while (!IsNil(list)) {
    Val item = Head(list);
    if (IsList(item)) {
      while (!IsNil(item)) {
        result = MakePair(Head(item), result);
        item = Tail(item);
      }
    } else {
      result = MakePair(item, result);
    }
    list = Tail(list);
  }
  return Reverse(result);
}

u32 ListLength(Val list)
{
  u32 length = 0;
  while (!IsNil(list)) {
    length++;
    list = Tail(list);
  }
  return length;
}

Val ListAt(Val list, u32 index)
{
  if (IsNil(list)) return nil;
  if (index == 0) return Head(list);
  return ListAt(Tail(list), index - 1);
}

Val First(Val list)
{
  return ListAt(list, 0);
}

Val Second(Val list)
{
  return ListAt(list, 1);
}

Val Third(Val list)
{
  return ListAt(list, 2);
}

Val MakeTuple(u32 count, ...)
{
  if (mem_next + count + 1 >= MEM_SIZE) Error("Out of memory");

  Val tuple = TupleVal(mem_next);
  mem[mem_next++] = TupHdr(count);

  if (count == 0) {
    mem[mem_next++] = nil;
    return tuple;
  }

  va_list args;
  va_start(args, count);
  for (u32 i = 0; i < count; i++) {
    Val arg = va_arg(args, Val);
    mem[mem_next++] = arg;
  }
  va_end(args);

  return tuple;
}

Val ListToTuple(Val list)
{
  u32 count = ListLength(list);
  if (mem_next + count + 1 >= MEM_SIZE) Error("Out of memory");

  Val tuple = TupleVal(mem_next);
  mem[mem_next++] = TupHdr(count);

  if (count == 0) {
    mem[mem_next++] = nil;
    return tuple;
  }

  while (!IsNil(list)) {
    mem[mem_next++] = Head(list);
    list = Tail(list);
  }

  return tuple;
}

u32 TupleLength(Val tuple)
{
  u32 index = RawVal(tuple);
  return HdrVal(mem[index]);
}

Val TupleAt(Val tuple, u32 i)
{
  u32 index = RawVal(tuple);
  return mem[(index + i + 1) % MEM_SIZE];
}

void TupleSet(Val tuple, u32 i, Val val)
{
  u32 index = RawVal(tuple);
  mem[(index + i + 1) % MEM_SIZE] = val;
}

bool IsTagged(Val exp, char *tag)
{
  if (IsPair(exp) && Eq(Head(exp), SymbolFor(tag))) return true;
  if (IsTuple(exp) && Eq(TupleAt(exp, 0), SymbolFor(tag))) return true;
  return false;
}

Val MakeSymbol(char *src)
{
  return MakeSymbolFromSlice(src, strlen(src));
}

Val MakeSymbolFromSlice(char *src, u32 len)
{
  Val key = SymVal(Hash(src, len));

  for (u32 i = 0; i < sym_next; i++) {
    if (Eq(symbols[i].key, key)) {
      return key;
    }
  }

  if (sym_next >= NUM_SYMBOLS) Error("Too many symbols");

  Symbol *sym = &symbols[sym_next++];
  sym->key = key;
  sym->name = malloc(len+1);
  memcpy(sym->name, src, len);
  sym->name[len] = '\0';
  return key;
}

Val BoolSymbol(bool val)
{
  if (val) {
    return MakeSymbol("true");
  } else {
    return MakeSymbol("false");
  }
}

Val SymbolFor(char *src)
{
  return SymVal(Hash(src, strlen(src)));
}

char *SymbolName(Val sym)
{
  for (u32 i = 0; i < sym_next; i++) {
    if (Eq(symbols[i].key, sym)) {
      return symbols[i].name;
    }
  }
  return NULL;
}

void DumpSymbols(void)
{
  printf("Symbols\n");
  for (u32 i = 0; i < sym_next; i++) {
    printf("  0x%0X %s\n", symbols[i].key.as_v, symbols[i].name);
  }
}

Val MakeBinary(char *src, u32 len)
{
  u32 count = (len - 1) / 4 + 1;

  if (mem_next + count + 1 >= MEM_SIZE) Error("Out of memory");

  Val binary = BinVal(mem_next);
  mem[mem_next++] = BinHdr(len);

  u8 *data = (u8*)&mem[mem_next];
  for (u32 i = 0; i < len; i++) {
    data[i] = src[i];
  }

  mem_next += count;

  return binary;
}

u32 BinaryLength(Val binary)
{
  u32 index = RawVal(binary);
  return HdrVal(mem[index]);
}

char *BinaryData(Val binary)
{
  u32 index = RawVal(binary);
  return (char*)&mem[index+1];
}

char *BinToCStr(Val binary)
{
  u32 len = BinaryLength(binary);
  char *src = BinaryData(binary);
  char *dst = malloc(len+1);
  for (u32 i = 0; i < len; i++) {
    dst[i] = src[i];
  }
  dst[len] = '\0';
  return dst;
}

u32 HashBinary(Val binary)
{
  u32 length = BinaryLength(binary);
  return Hash(BinaryData(binary), length);
}

void DictSet(Val dict, Val key, Val value);

Val MakeDict(Val keys, Val vals)
{
  u32 size = ListLength(keys);

  Val dict = MakeEmptyDict(size);

  while (!IsNil(keys)) {
    Val key = Head(keys);
    Val value = Head(vals);
    DictSet(dict, key, value);
    keys = Tail(keys);
    vals = Tail(vals);
  }

  return dict;
}

Val MakeEmptyDict(u32 size)
{
  Val dict = DictVal(mem_next);
  mem[mem_next++] = DctHdr(size);

  for (u32 i = 0; i < size; i++) {
    mem[mem_next++] = nil;
    mem[mem_next++] = nil;
  }

  return dict;
}

u32 DictSize(Val dict)
{
  u32 index = RawVal(dict);
  return HdrVal(mem[index]);
}

bool DictHasKey(Val dict, Val key)
{
  if (IsNil(key) || (!IsSym(key) && !IsBin(key))) {
    return false;
  }

  Val *data = &mem[(u32)RawVal(dict)];
  u32 size = HdrVal(data[0]);
  u32 hash = (IsBin(key)) ? HashBinary(key) : RawVal(key);
  u32 index = hash % size;

  u32 start = index;
  u32 key_slot = 1 + 2*index;
  while (!IsNil(data[key_slot])) {
    if (Eq(data[key_slot], key)) {
      return true;
    }

    index = (index + 1) % size;
    if (index == start) {
      return false;
    }
    key_slot = 1 + 2*index;
  }

  return false;
}

Val DictGet(Val dict, Val key)
{
  if (IsNil(key) || (!IsSym(key) && !IsBin(key))) {
    Error("Invalid key: %s", ValStr(key));
  }

  Val *data = &mem[(u32)RawVal(dict)];
  u32 size = HdrVal(data[0]);
  u32 hash = (IsBin(key)) ? HashBinary(key) : RawVal(key);
  u32 index = hash % size;

  u32 start = index;
  u32 key_slot = 1 + 2*index;
  while (!IsNil(data[key_slot])) {
    if (Eq(data[key_slot], key)) {
      return data[key_slot+1];
    }

    index = (index + 1) % size;
    if (index == start) {
      return nil;
    }
    key_slot = 1 + 2*index;
  }

  return nil;
}

Val DictKeyAt(Val dict, u32 i)
{
  u32 index = RawVal(dict);
  return mem[index+1+2*i];
}

Val DictValueAt(Val dict, u32 i)
{
  u32 index = RawVal(dict);
  return mem[index+1+2*i+1];
}

void DictSet(Val dict, Val key, Val value)
{
  if (IsNil(key) || (!IsSym(key) && !IsBin(key))) {
    Error("Invalid key: %s", ValStr(key));
  }

  Val *data = &mem[(u32)RawVal(dict)];
  u32 size = HdrVal(data[0]);
  u32 hash = (IsBin(key)) ? HashBinary(key) : RawVal(key);
  u32 index = hash % size;

  u32 start = index;
  u32 key_slot = 1 + 2*index;
  while (!IsNil(data[key_slot]) && !Eq(data[key_slot], key)) {
    index = (index + 1) % size;
    if (index == start) Error("Dict full");
    key_slot = 1 + 2*index;
  }

  data[key_slot] = key;
  data[key_slot+1] = value;
}
