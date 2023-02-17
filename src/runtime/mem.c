#include "mem.h"
#include "../hash.h"
#include "../vec.h"
#include "env.h"
#include "print.h"
#include "../platform/error.h"
#include "../platform/console.h"

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
  Val tuple = TupleVal(VecCount(*mem));
  VecPush(*mem, TupHdr(count));

  for (u32 i = 0; i < count; i++) {
    VecPush(*mem, nil);
  }

  return tuple;
}

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

void PrintHeap(Val *mem, StringMap *strings)
{
  Print("───╴Heap╶───\n");

  for (u32 i = 0; i < VecCount(mem) && i < 100; i++) {
    Print("%4u │ ", i);
    PrintVal(mem, strings, mem[i]);
    Print("\n");
  }
}
