#include "mem.h"
#include "univ/system.h"
#include "univ/math.h"
#include "univ/hash.h"

#ifdef DEBUG
#include <stdio.h>
#endif

Val FloatVal(float num)
{
  union {Val as_v; float as_f;} convert;
  convert.as_f = num;
  return convert.as_v;
}

float RawFloat(Val value)
{
  union {Val as_v; float as_f;} convert;
  convert.as_v = value;
  return convert.as_f;
}

void InitMem(Mem *mem, u32 capacity)
{
  mem->values = Alloc(sizeof(Val) * capacity);
  mem->count = 0;
  mem->capacity = capacity;
  Pair(Nil, Nil, mem);
}

void DestroyMem(Mem *mem)
{
  if (mem->values) Free(mem->values);
  mem->count = 0;
  mem->capacity = 0;
  mem->values = 0;
}

void PushMem(Mem *mem, Val value)
{
  mem->values[mem->count++] = value;
}

Val PopMem(Mem *mem)
{
  return mem->values[--mem->count];
}

Val TypeOf(Val value, Mem *mem)
{
  if (IsFloat(value)) return FloatType;
  if (IsInt(value)) return IntType;
  if (IsSym(value)) return SymType;
  if (IsPair(value)) return PairType;
  if (IsTuple(value, mem)) return TupleType;
  if (IsBinary(value, mem)) return BinaryType;
  return Nil;
}

Val Pair(Val head, Val tail, Mem *mem)
{
  Val pair = PairVal(mem->count);
  if (!CheckCapacity(mem, 2)) ResizeMem(mem, 2*mem->capacity);
  Assert(CheckCapacity(mem, 2));
  PushMem(mem, head);
  PushMem(mem, tail);
  return pair;
}

Val Head(Val pair, Mem *mem)
{
  Assert(IsPair(pair));
  return mem->values[RawVal(pair)];
}

Val Tail(Val pair, Mem *mem)
{
  Assert(IsPair(pair));
  return mem->values[RawVal(pair)+1];
}

void SetHead(Val pair, Val value, Mem *mem)
{
  mem->values[RawVal(pair)] = value;
}

void SetTail(Val pair, Val value, Mem *mem)
{
  mem->values[RawVal(pair)+1] = value;
}

u32 ListLength(Val list, Mem *mem)
{
  u32 length = 0;
  while (IsPair(list) && list != Nil) {
    length++;
    list = Tail(list, mem);
  }
  return length;
}

bool ListContains(Val list, Val item, Mem *mem)
{
  while (IsPair(list) && list != Nil) {
    if (Head(list, mem) == item) return true;
    list = Tail(list, mem);
  }
  return false;
}

Val ReverseList(Val list, Val tail, Mem *mem)
{
  Assert(IsPair(list) && IsPair(tail));
  while (list != Nil) {
    tail = Pair(Head(list, mem), tail, mem);
    list = Tail(list, mem);
  }
  return tail;
}

Val ListGet(Val list, u32 index, Mem *mem)
{
  Assert(IsPair(list));
  while (index > 0) {
    list = Tail(list, mem);
    if (list == Nil || !IsPair(list)) return Nil;
    index--;
  }
  return Head(list, mem);
}

Val ListCat(Val list1, Val list2, Mem *mem)
{
  Assert(IsPair(list1) && IsPair(list2));
  list1 = ReverseList(list1, Nil, mem);
  return ReverseList(list1, list2, mem);
}

Val MakeTuple(u32 length, Mem *mem)
{
  u32 i;
  Val tuple = ObjVal(mem->count);
  if (!CheckCapacity(mem, length+1)) ResizeMem(mem, Max(2*mem->capacity, mem->count + length + 1));
  Assert(CheckCapacity(mem, length + 1));
  PushMem(mem, TupleHeader(length));
  for (i = 0; i < length; i++) {
    PushMem(mem, Nil);
  }
  return tuple;
}

bool IsTuple(Val value, Mem *mem)
{
  return IsObj(value) && IsTupleHeader(mem->values[RawVal(value)]);
}

u32 TupleLength(Val tuple, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  return RawVal(mem->values[RawVal(tuple)]);
}

bool TupleContains(Val tuple, Val item, Mem *mem)
{
  u32 i;
  Assert(IsTuple(tuple, mem));
  for (i = 0; i < TupleLength(tuple, mem); i++) {
    if (TupleGet(tuple, i, mem) == item) return true;
  }

  return false;
}

void TupleSet(Val tuple, u32 i, Val value, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  Assert(i < TupleLength(tuple, mem));
  mem->values[RawVal(tuple) + i + 1] = value;
}

Val TupleGet(Val tuple, u32 i, Mem *mem)
{
  return mem->values[RawVal(tuple) + i + 1];
}

Val TupleCat(Val tuple1, Val tuple2, Mem *mem)
{
  Val result;
  u32 len1, len2, i;
  Assert(IsTuple(tuple1, mem) && IsTuple(tuple2, mem));
  len1 = TupleLength(tuple1, mem);
  len2 = TupleLength(tuple2, mem);
  result = MakeTuple(len1+len2, mem);
  for (i = 0; i < len1; i++) {
    TupleSet(result, i, TupleGet(tuple1, i, mem), mem);
  }
  for (i = 0; i < len2; i++) {
    TupleSet(result, len1+i, TupleGet(tuple1, i, mem), mem);
  }
  return result;
}

Val MakeBinary(u32 size, Mem *mem)
{
  Val binary = ObjVal(mem->count);
  u32 cells = (size == 0) ? 1 : NumBinCells(size);
  u32 i;

  if (!CheckCapacity(mem, cells+1)) ResizeMem(mem, Max(2*mem->capacity, mem->count+cells+1));
  Assert(CheckCapacity(mem, cells + 1));
  PushMem(mem, BinaryHeader(size));

  for (i = 0; i < cells; i++) {
    PushMem(mem, 0);
  }
  return binary;
}

Val BinaryFrom(char *str, u32 size, Mem *mem)
{
  Val bin = MakeBinary(size, mem);
  char *data = BinaryData(bin, mem);
  Copy(str, data, size);
  return bin;
}

bool IsBinary(Val value, Mem *mem)
{
  return IsObj(value) && IsBinaryHeader(mem->values[RawVal(value)]);
}

u32 BinaryLength(Val binary, Mem *mem)
{
  Assert(IsBinary(binary, mem));
  return RawVal(mem->values[RawVal(binary)]);
}

bool BinaryContains(Val binary, Val item, Mem *mem)
{
  u8 *bin_data, *item_data;
  Assert(IsBinary(binary, mem));
  bin_data = BinaryData(binary, mem);
  item_data = BinaryData(item, mem);

  if (IsInt(item)) {
    u32 i;
    u8 byte;
    if (RawInt(item) < 0 || RawInt(item) > 255) return false;
    byte = RawInt(item);
    for (i = 0; i < BinaryLength(binary, mem); i++) {
      if (bin_data[i] == byte) return true;
    }
  } else if (IsBinary(item, mem)) {
    u32 i, j, item_hash, test_hash;
    u32 item_length = BinaryLength(item, mem);
    u32 bin_length = BinaryLength(binary, mem);
    if (item_length > bin_length) return false;
    if (item_length == 0 || bin_length == 0) return false;

    item_hash = Hash(item_data, item_length);
    test_hash = Hash(bin_data, item_length);

    for (i = 0; i < bin_length - item_length; i++) {
      if (test_hash == item_hash) break;
      test_hash = SkipHash(test_hash, bin_data[i], item_length);
      test_hash = AppendHash(test_hash, bin_data[i+item_length]);
    }

    if (test_hash == item_hash) {
      for (j = 0; j < item_length; j++) {
        if (bin_data[i+j] != item_data[j]) return false;
      }
      return true;
    }
  }

  return false;
}

void *BinaryData(Val binary, Mem *mem)
{
  Assert(IsBinary(binary, mem));
  return &mem->values[RawVal(binary)+1];
}

Val BinaryCat(Val binary1, Val binary2, Mem *mem)
{
  Val result;
  u32 len1, len2;
  u8 *data;
  Assert(IsBinary(binary1, mem) && IsBinary(binary2, mem));
  len1 = BinaryLength(binary1, mem);
  len2 = BinaryLength(binary2, mem);
  result = MakeBinary(len1+len2, mem);
  data = BinaryData(result, mem);
  Copy(BinaryData(binary1, mem), data, len1);
  Copy(BinaryData(binary2, mem), data+len1, len2);
  return result;
}

#define NodeCount(header)     PopCount((header) & 0xFFFF)
#define SlotNum(hash, n)      (((hash) >> (4*n)) & 0x0F)
#define IndexNum(header, n)   PopCount((header) & (Bit(n) - 1))
#define GetNode(node, i, mem) ((mem)->values[RawVal(node)+1+(i)])

static Val MakeMapNode(u32 header, Mem *mem)
{
  Val map = ObjVal(mem->count);
  u32 num_children = NodeCount(header);
  PushMem(mem, header);
  if (num_children == 0) {
    PushMem(mem, Nil);
  } else {
    u32 i;
    for (i = 0; i < num_children; i++) PushMem(mem, Nil);
  }
  return map;
}

Val MakeMap(Mem *mem)
{
  return MakeMapNode(MapHeader(0), mem);
}

bool IsMap(Val value, Mem *mem)
{
  return IsObj(value) && IsMapHeader(mem->values[RawVal(value)]);
}

u32 MapCount(Val map, Mem *mem)
{
  Val header = mem->values[RawVal(map)];
  u32 count = 0;
  u32 num_children = NodeCount(header);
  u32 i;
  for (i = 0; i < num_children; i++) {
    Val child = GetNode(map, i, mem);
    if (IsPair(child)) {
      count++;
    } else {
      count += MapCount(child, mem);
    }
  }
  return count;
}

static bool NodeContains(Val node, Val key, u32 level, Mem *mem)
{
  Val header = mem->values[RawVal(node)];
  u32 hash = Hash(&key, sizeof(Val));
  u32 slot = SlotNum(hash, level);
  u32 index = IndexNum(header, slot);

  if ((header & Bit(slot)) == 0) return false;

  node = GetNode(node, index, mem);
  if (IsPair(node)) {
    return key == Head(node, mem);
  } else {
    return NodeContains(node, key, level+1, mem);
  }
}

bool MapContains(Val map, Val key, Mem *mem)
{
  return NodeContains(map, key, 0, mem);
}

static Val MakeInternalNode(Val child1, Val child2, u32 level, Mem *mem)
{
  Val key1 = Head(child1, mem);
  u32 hash1 = Hash(&key1, sizeof(Val));
  u32 slot1 = SlotNum(hash1, level);
  Val key2 = Head(child2, mem);
  u32 hash2 = Hash(&key2, sizeof(Val));
  u32 slot2 = SlotNum(hash2, level);
  Val node = MakeMapNode(MapHeader(Bit(slot1) | Bit(slot2)), mem);

  if (slot1 == slot2) {
    GetNode(node, 0, mem) = MakeInternalNode(child1, child2, level+1, mem);
  } else if (slot1 < slot2) {
    GetNode(node, 0, mem) = child1;
    GetNode(node, 1, mem) = child2;
  } else {
    GetNode(node, 0, mem) = child2;
    GetNode(node, 1, mem) = child1;
  }
  return node;
}

static Val NodeInsert(Val node, u32 slot, Val child, Mem *mem)
{
  Val header = mem->values[RawVal(node)];
  u32 num_children = NodeCount(header);
  u32 index = IndexNum(header, slot);
  bool replace = (header & Bit(slot)) == 1;
  u32 i;
  Val new_node = MakeMapNode(MapHeader(header | Bit(slot)), mem);
  for (i = 0; i < index; i++) {
    GetNode(new_node, i, mem) = GetNode(node, i, mem);
  }
  GetNode(new_node, index, mem) = child;
  for (i = index + replace; i < num_children; i++) {
    GetNode(new_node, index+i+1, mem) = GetNode(node, i, mem);
  }
  return new_node;
}

static Val NodeSet(Val node, Val key, Val value, u32 level, Mem *mem)
{
  Val header = mem->values[RawVal(node)];
  u32 hash = Hash(&key, sizeof(Val));
  u32 slot = SlotNum(hash, level);
  u32 index = IndexNum(header, slot);

  if ((header & Bit(slot)) == 0) {
    Val pair = Pair(key, value, mem);
    return NodeInsert(node, slot, pair, mem);
  } else {
    Val child = GetNode(node, index, mem);
    if (!IsPair(child)) {
      Val new_child = NodeSet(child, key, value, level + 1, mem);
      return NodeInsert(node, slot, new_child, mem);
    } else if (key == Head(child, mem)) {
      Val pair = Pair(key, value, mem);
      return NodeInsert(node, slot, pair, mem);
    } else {
      Val pair = Pair(key, value, mem);
      Val new_child = MakeInternalNode(child, pair, level + 1, mem);
      return NodeInsert(node, slot, new_child, mem);
    }
  }
}

Val MapSet(Val map, Val key, Val value, Mem *mem)
{
  return NodeSet(map, key, value, 0, mem);
}

static bool NodeGet(Val node, Val key, u32 level, Mem *mem)
{
  Val header = mem->values[RawVal(node)];
  u32 hash = Hash(&key, sizeof(Val));
  u32 slot = SlotNum(hash, level);
  u32 index = IndexNum(header, slot);

  if ((header & Bit(slot)) == 0) return Nil;

  node = GetNode(node, index, mem);
  if (IsPair(node)) {
    if (key == Head(node, mem)) {
      return Tail(node, mem);
    } else {
      return Nil;
    }
  } else {
    return NodeGet(node, key, level+1, mem);
  }
}

Val MapGet(Val map, Val key, Mem *mem)
{
  return NodeGet(map, key, 0, mem);
}

Val MapMerge(Val map1, Val map2, Mem *mem)
{
  u32 i;
  if (MapCount(map2, mem) == 0) return map1;
  if (MapCount(map1, mem) == 0) return map2;

  for (i = 0; i < MapCount(map2, mem); i++) {
    Val child = GetNode(map2, i, mem);
    if (IsPair(child)) {
      map1 = MapSet(map1, Head(child, mem), Tail(child, mem), mem);
    } else {
      map1 = MapMerge(map1, child, mem);
    }
  }

  return map1;
}

bool CheckCapacity(Mem *mem, u32 amount)
{
  return mem->count + amount <= mem->capacity;
}

void ResizeMem(Mem *mem, u32 capacity)
{
  mem->capacity = capacity;
  mem->values = Realloc(mem->values, sizeof(Val)*capacity);
}

static Val ValSize(Val value)
{
  if (IsTupleHeader(value)) {
    return Max(2, RawVal(value) + 1);
  } else if (IsBinaryHeader(value)) {
    return Max(2, NumBinCells(RawVal(value)) + 1);
  } else if (IsMapHeader(value)) {
    return Max(2, PopCount(RawVal(value)) + 1);
  } else {
    return 1;
  }
}

static Val CopyValue(Val value, Mem *from, Mem *to)
{
  Val new_val = Nil;

  if (value == Nil) return Nil;
  if (!IsObj(value) && !IsPair(value)) return value;

  if (from->values[RawVal(value)] == Moved) {
    return from->values[RawVal(value)+1];
  }

  if (IsPair(value)) {
    new_val = Pair(Head(value, from), Tail(value, from), to);
  } else if (IsObj(value)) {
    u32 i;
    new_val = ObjVal(to->count);
    for (i = 0; i < ValSize(from->values[RawVal(value)]); i++) {
      PushMem(to, from->values[RawVal(value)+i]);
    }
  }

  from->values[RawVal(value)] = Moved;
  from->values[RawVal(value)+1] = new_val;
  return new_val;
}

void CollectGarbage(Val *roots, u32 num_roots, Mem *mem)
{
  u32 i;
  Mem new_mem;

#ifdef DEBUG
  printf("GARBAGE DAY!!!\n");
#endif

  InitMem(&new_mem, mem->capacity);

  for (i = 0; i < num_roots; i++) {
    roots[i] = CopyValue(roots[i], mem, &new_mem);
  }

  i = 2;
  while (i < new_mem.count) {
    Val value = new_mem.values[i];
    if (IsBinaryHeader(value)) {
      i += ValSize(value);
    } else {
      new_mem.values[i] = CopyValue(new_mem.values[i], mem, &new_mem);
      i++;
    }
  }

  Free(mem->values);
  *mem = new_mem;
}
