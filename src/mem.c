#include "mem.h"
#include "print.h"
#include <univ/vec.h>
#include <univ/hash.h>
#include <univ/io.h>
#include <univ/str.h>

void InitMem(Val **mem)
{
  *mem = NULL;
  VecPush(*mem, nil);
  VecPush(*mem, nil);
}

void InitHeap(Heap *heap)
{
  InitMem(&heap->mem);
  heap->symbols = MakeMap(&heap->mem, 32);
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
  u32 index = RawPair(pair);
  return mem[index];
}

Val Tail(Val *mem, Val pair)
{
  if (IsNil(pair)) return nil;
  u32 index = RawPair(pair);
  return mem[index+1];
}

void SetHead(Val **mem, Val pair, Val val)
{
  Assert(!IsNil(pair));
  u32 index = RawPair(pair);
  (*mem)[index] = val;
}

void SetTail(Val **mem, Val pair, Val val)
{
  Assert(!IsNil(pair));
  u32 index = RawPair(pair);
  (*mem)[index+1] = val;
}

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

Val MakeTuple(Val **mem, u32 count)
{
  Val tuple = ObjVal(VecCount(*mem));
  VecPush(*mem, TupleHeader(count));

  for (u32 i = 0; i < count; i++) {
    VecPush(*mem, nil);
  }

  return tuple;
}

u32 TupleLength(Val *mem, Val tuple)
{
  u32 index = RawObj(tuple);
  return HeaderVal(mem[index]);
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

Val MakeBinary(Val **mem, char *src, u32 len)
{
  Val bin = ObjVal(VecCount(*mem));
  VecPush(*mem, BinaryHeader(len));

  u8 *data = BinaryData(*mem, bin);
  for (u32 i = 0; i < len; i++) {
    data[i] = src[i];
  }

  u32 num_slots = (len - 1) / 4 + 1;
  GrowVec(*mem, num_slots);

  return bin;
}

u32 BinaryLength(Val *mem, Val bin)
{
  u32 index = RawObj(bin);
  return HeaderVal(mem[index]);
}

u8 *BinaryData(Val *mem, Val bin)
{
  u32 index = RawObj(bin);
  return (u8*)(&mem[index + 1]);
}

Val SymbolFor(char *src)
{
  u32 hash = HashBits(src, StrLen(src), valBits);
  return SymVal(hash);
}

Val SymbolFrom(char *src, u32 length)
{
  u32 hash = HashBits(src, length, valBits);
  return SymVal(hash);
}

Val BinToSym(Val *mem, Val bin)
{
  u32 hash = HashBits(BinaryData(mem, bin), BinaryLength(mem, bin), valBits);
  return SymVal(hash);
}

Val MakeMap(Val **mem, u32 count)
{
  Val map = ObjVal(VecCount(*mem));

  VecPush(*mem, DictHeader(count));

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
  return HeaderVal(mem[RawObj(map)]);
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

static void PrintTreeLevel(Val *mem, Val symbols, Val root, u32 indent);

static void PrintTreeList(Val *mem, Val symbols, Val list, u32 indent)
{
  if (IsNil(list)) return;

  PrintTreeLevel(mem, symbols, Head(mem, list), indent);
  if (IsPair(Tail(mem, list))) {
    PrintTreeList(mem, symbols, Tail(mem, list), indent);
  } else {
    Print("| ");
    PrintTreeLevel(mem, symbols, Tail(mem, list), indent);
  }
}

static void PrintTreeLevel(Val *mem, Val symbols, Val root, u32 indent)
{
  for (u32 i = 0; i < indent; i++) Print("  ");
  PrintVal(mem, symbols, root);
  Print("\n");
  if (IsPair(root)) {
    PrintTreeList(mem, symbols, root, indent + 1);
  }
}

void PrintTree(Val *mem, Val symbols, Val root)
{
  PrintTreeLevel(mem, symbols, root, 0);
}
