#include "mem.h"
#include "printer.h"
#include "hash.h"
#include "env.h"
#include "vec.h"

void InitMem(Val *mem)
{
  VecPush(mem, nil);
  VecPush(mem, nil);
}

Val MakePair(Val **mem, Val head, Val tail)
{
  Val pair = PairVal(VecCount(*mem));

  VecPush(*mem, head);
  VecPush(*mem, tail);

  return pair;
}

Val Head(Val *mem, Val pair)
{
  if (IsNil(pair)) return nil;
  u32 index = RawObj(pair);
  return mem[index];
}

Val Tail(Val *mem, Val pair)
{
  if (IsNil(pair)) return nil;
  u32 index = RawObj(pair);
  return mem[index+1];
}

void SetHead(Val **mem, Val pair, Val val)
{
  if (IsNil(pair)) Fatal("Can't change nil");
  u32 index = RawObj(pair);
  (*mem)[index] = val;
}

void SetTail(Val **mem, Val pair, Val val)
{
  if (IsNil(pair)) Fatal("Can't change nil");
  u32 index = RawObj(pair);
  (*mem)[index+1] = val;
}

Val MakeList(Val **mem, u32 length, ...)
{
  Val list = nil;
  va_list args;
  va_start(args, length);
  for (u32 i = 0; i < length; i++) {
    Val arg = va_arg(args, Val);
    list = MakePair(mem, arg, list);
  }
  va_end(args);
  return Reverse(mem, list);
}

// Val MakeTagged(Val *mem, u32 length, char *name, ...)
// {
//   Val list = MakePair(mem, MakeSymbol(name), nil);
//   va_list args;
//   va_start(args, name);
//   for (u32 i = 0; i < length - 1; i++) {
//     Val arg = va_arg(args, Val);
//     list = MakePair(mem, arg, list);
//   }
//   va_end(args);
//   return Reverse(mem, list);
// }

Val ReverseOnto(Val **mem, Val list, Val tail)
{
  if (IsNil(list)) return tail;

  Val rest = Tail(*mem, list);
  SetTail(mem, list, tail);
  return ReverseOnto(mem, rest, list);
}

Val Reverse(Val **mem, Val list)
{
  return ReverseOnto(mem, list, nil);
}

// Val Flatten(Val *mem, Val list)
// {
//   Val result = nil;
//   while (!IsNil(list)) {
//     Val item = Head(mem, list);
//     if (IsList(mem, item)) {
//       while (!IsNil(item)) {
//         result = MakePair(mem, Head(mem, item), result);
//         item = Tail(mem, item);
//       }
//     } else {
//       result = MakePair(mem, item, result);
//     }
//     list = Tail(mem, list);
//   }
//   return Reverse(mem, result);
// }

u32 ListLength(Val *mem, Val list)
{
  u32 length = 0;
  while (!IsNil(list)) {
    length++;
    list = Tail(mem, list);
  }
  return length;
}

Val ListAt(Val *mem, Val list, u32 index)
{
  if (IsNil(list)) return nil;
  if (index == 0) return Head(mem, list);
  return ListAt(mem, Tail(mem, list), index - 1);
}

Val First(Val *mem, Val list)
{
  return ListAt(mem, list, 0);
}

Val Second(Val *mem, Val list)
{
  return ListAt(mem, list, 1);
}

Val Third(Val *mem, Val list)
{
  return ListAt(mem, list, 2);
}

Val ListLast(Val *mem, Val list)
{
  while (!IsNil(Tail(mem, list))) {
    list = Tail(mem, list);
  }
  return Head(mem, list);
}

void ListAppend(Val **mem, Val list1, Val list2)
{
  while (!IsNil(Tail(*mem, list1))) {
    list1 = Tail(*mem, list1);
  }

  SetTail(mem, list1, list2);
}

Val MakeTuple(Val **mem, u32 count, ...)
{
  Val tuple = TupleVal(VecCount(*mem));
  VecPush(*mem, TupHdr(count));

  if (count == 0) {
    VecPush(*mem, nil);
    return tuple;
  }

  va_list args;
  va_start(args, count);
  for (u32 i = 0; i < count; i++) {
    Val arg = va_arg(args, Val);
    VecPush(*mem, arg);
  }
  va_end(args);

  return tuple;
}

// static Val MakeEmptyTuple(Val *mem, u32 count)
// {
//   Val tuple = TupleVal(VecCount(mem));
//   VecPush(mem, TupHdr(count));

//   for (u32 i = 0; i < count; i++) {
//     VecPush(mem, nil);
//   }

//   return tuple;
// }

u32 TupleLength(Val *mem, Val tuple)
{
  u32 index = RawObj(tuple);
  return HdrVal(mem[index]);
}

Val TupleAt(Val *mem, Val tuple, u32 i)
{
  u32 index = RawObj(tuple);
  return mem[index + i + 1];
}

void TupleSet(Val *mem, Val tuple, u32 i, Val val)
{
  u32 index = RawObj(tuple);
  mem[index + i + 1] = val;
}

// bool IsTagged(Val *mem, Val exp, char *tag)
// {
//   if (IsPair(exp) && Eq(Head(mem, exp), SymbolFor(tag))) return true;
//   if (IsTuple(exp) && Eq(TupleAt(mem, exp, 0), SymbolFor(tag))) return true;
//   return false;
// }

Val MakeSymbol(Symbol **symbols, char *src)
{
  return MakeSymbolFromSlice(symbols, src, strlen(src));
}

Val MakeSymbolFromSlice(Symbol **symbols, char *src, u32 len)
{
  Val key = SymVal(Hash(src, len));

  for (u32 i = 0; i < VecCount(*symbols); i++) {
    if (Eq((*symbols)[i].key, key)) {
      return key;
    }
  }

  Symbol sym;
  sym.key = key;
  sym.name = malloc(len+1);
  memcpy(sym.name, src, len);
  sym.name[len] = '\0';

  VecPush(*symbols, sym);

  return key;
}

Val SymbolFor(char *src)
{
  return SymVal(Hash(src, strlen(src)));
}

char *SymbolName(Symbol *symbols, Val sym)
{
  for (u32 i = 0; i < VecCount(symbols); i++) {
    if (Eq(symbols[i].key, sym)) {
      return symbols[i].name;
    }
  }
  return NULL;
}

void DumpSymbols(Symbol *symbols)
{
  printf("Symbols\n");
  for (u32 i = 0; i < VecCount(symbols); i++) {
    printf("  0x%0X %s\n", symbols[i].key.as_v, symbols[i].name);
  }
}

Val MakeBinary(Val *mem, char *src, u32 len)
{
  u32 words = (len - 1) / 4 + 1;

  Val binary = BinVal(VecCount(mem));
  VecPush(mem, BinHdr(len));

  u8 *data = (u8*)&mem[VecCount(mem)];
  GrowVec(mem, words);

  for (u32 i = 0; i < len; i++) {
    data[i] = src[i];
  }

  return binary;
}

u32 BinaryLength(Val *mem, Val binary)
{
  u32 index = RawObj(binary);
  return HdrVal(mem[index]);
}

char *BinaryData(Val *mem, Val binary)
{
  u32 index = RawObj(binary);
  return (char*)&mem[index+1];
}

char *BinToCStr(Val *mem, Val binary)
{
  u32 len = BinaryLength(mem, binary);
  char *src = BinaryData(mem, binary);
  char *dst = malloc(len+1);
  for (u32 i = 0; i < len; i++) {
    dst[i] = src[i];
  }
  dst[len] = '\0';
  return dst;
}

Val BinaryAt(Val *mem, Val binary, u32 i)
{
  if (i >= BinaryLength(mem, binary)) return nil;

  return IntVal(BinaryData(mem, binary)[i]);
}

Val MakeMap(Val **mem, u32 count)
{
  Val map = MapVal(VecCount(*mem));

  VecPush(*mem, MapHdr(count));

  for (u32 i = 0; i < count; i++) {
    VecPush(*mem, nil);
    VecPush(*mem, nil);
  }

  return map;
}

void MapPut(Val *mem, Val map, Val key, Val val)
{
  u32 size = MapSize(mem, map);
  u32 base = RawObj(map) + 1;
  u32 index = RawObj(key) % size;

  while (!IsNil(MapKeyAt(mem, map, index))) {
    if (Eq(MapKeyAt(mem, map, index), key)) {
      mem[base+index*2+1] = val;
      return;
    }

    index = (index + 1) % size;
  }

  mem[base+index*2] = key;
  mem[base+index*2+1] = val;
}

bool MapHasKey(Val *mem, Val map, Val key)
{
  u32 size = MapSize(mem, map);
  u32 base = RawObj(map) + 1;
  u32 index = (RawObj(key) * 2) % size;

  while (!IsNil(mem[base+index])) {
    if (Eq(MapKeyAt(mem, map, index), key)) {
      return true;
    }

    index = (index + 1) % size;
  }

  return false;
}

Val MapGet(Val *mem, Val map, Val key)
{
  u32 size = MapSize(mem, map);
  u32 index = RawObj(key) % size;

  while (!IsNil(MapKeyAt(mem, map, index))) {
    if (Eq(MapKeyAt(mem, map, index), key)) {
      return MapValAt(mem, map, index);
    }

    index = (index + 1) % size;
  }

  return nil;
}

u32 MapSize(Val *mem, Val map)
{
  return HdrVal(mem[RawObj(map)]);
}

Val MapKeyAt(Val *mem, Val map, u32 i)
{
  u32 base = RawObj(map) + 1;
  return mem[base + i*2];
}

Val MapValAt(Val *mem, Val map, u32 i)
{
  u32 base = RawObj(map) + 1;
  return mem[base + i*2 + 1];
}

void PrintHeap(Val *mem, Symbol *symbols)
{
  printf("───╴Heap╶───\n");

  for (u32 i = 0; i < VecCount(mem) && i < 100; i++) {
    printf("%4u │ ", i);
    PrintVal(mem, symbols, mem[i]);
    printf("\n");
  }
}
