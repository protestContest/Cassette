#include "mem.h"
#include "univ/system.h"
#include "univ/math.h"
#include "univ/hash.h"
#include "univ/str.h"
#include "symbols.h"

Val FloatVal(float num)
{
  /* C is weird and I hate it */
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
  InitVec((Vec*)mem, sizeof(Val), capacity);
  /* Nil is defined as the first pair in memory, which references itself */
  Pair(Nil, Nil, mem);
}

#define PushMem(mem, val) (VecRef(mem, (mem)->count++) = (val))
#define PopMem(mem)       VecRef(mem, --(mem)->count)

Val TypeSym(Val value, Mem *mem)
{
  if (IsFloat(value)) return FloatType;
  if (IsInt(value)) return IntType;
  if (IsSym(value)) return SymType;
  if (IsPair(value)) return PairType;
  if (IsTuple(value, mem)) return TupleType;
  if (IsBinary(value, mem)) return BinaryType;
  if (IsMap(value, mem)) return MapType;
  return Nil;
}

char *TypeName(Val type)
{
  switch (type) {
  case FloatType:   return "float";
  case IntType:     return "integer";
  case SymType:     return "symbol";
  case PairType:    return "pair";
  case TupleType:   return "tuple";
  case BinaryType:  return "binary";
  case MapType:     return "map";
  default:          return "?";
  }
}

/*
A pair is just two values, without a header. Instead, pairs have their own
pointer type, the "pair".
*/

Val Pair(Val head, Val tail, Mem *mem)
{
  Val pair = PairVal(mem->count);
  if (CapacityLeft(mem) < 2) ResizeMem(mem, 2*mem->capacity);
  Assert(CapacityLeft(mem) >= 2);
  PushMem(mem, head);
  PushMem(mem, tail);
  return pair;
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
  while (list != Nil) {
    tail = Pair(Head(list, mem), tail, mem);
    list = Tail(list, mem);
  }
  return tail;
}

Val ListGet(Val list, u32 index, Mem *mem)
{
  while (index > 0) {
    list = Tail(list, mem);
    if (list == Nil || !IsPair(list)) return Nil;
    index--;
  }
  return Head(list, mem);
}

Val ListCat(Val list1, Val list2, Mem *mem)
{
  list1 = ReverseList(list1, Nil, mem);
  return ReverseList(list1, list2, mem);
}

/*
A tuple is a header followed by a fixed number of values. The header value is
the number of values in the tuple.
*/

Val MakeTuple(u32 length, Mem *mem)
{
  u32 i;
  Val tuple = ObjVal(mem->count);
  if (CapacityLeft(mem) < length+1) ResizeMem(mem, Max(2*mem->capacity, mem->count + length + 1));
  Assert(CapacityLeft(mem) >= length + 1);
  PushMem(mem, TupleHeader(length));
  for (i = 0; i < length; i++) {
    PushMem(mem, Nil);
  }
  return tuple;
}

bool TupleContains(Val tuple, Val item, Mem *mem)
{
  u32 i;
  for (i = 0; i < TupleLength(tuple, mem); i++) {
    if (TupleGet(tuple, i, mem) == item) return true;
  }

  return false;
}

void TupleSet(Val tuple, u32 i, Val value, Mem *mem)
{
  Assert(i < TupleLength(tuple, mem));
  VecRef(mem, RawVal(tuple) + i + 1) = value;
}

Val TupleCat(Val tuple1, Val tuple2, Mem *mem)
{
  Val result;
  u32 len1, len2, i;
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

/*
A binary is a header followed by raw binary data. The header value is the size
of the binary in bytes. Since memory is allocated in 4-byte cells, the last cell
is padded with zeroes.
*/

Val MakeBinary(u32 size, Mem *mem)
{
  Val binary = ObjVal(mem->count);
  u32 cells = (size == 0) ? 1 : NumBinCells(size);
  u32 i;
  if (CapacityLeft(mem) < cells+1) ResizeMem(mem, Max(2*mem->capacity, mem->count+cells+1));
  Assert(CapacityLeft(mem) >= cells + 1);
  PushMem(mem, BinaryHeader(size));
  for (i = 0; i < cells; i++) PushMem(mem, 0);
  return binary;
}

Val BinaryFrom(char *str, u32 size, Mem *mem)
{
  Val bin = MakeBinary(size, mem);
  char *data = BinaryData(bin, mem);
  Copy(str, data, size);
  return bin;
}

bool BinaryContains(Val binary, Val item, Mem *mem)
{
  char *bin_data = BinaryData(binary, mem);
  u32 bin_len = BinaryLength(binary, mem);

  if (IsInt(item)) {
    u32 i;
    u8 byte;
    if (RawInt(item) < 0 || RawInt(item) > 255) return false;
    byte = RawInt(item);
    for (i = 0; i < bin_len; i++) {
      if (bin_data[i] == byte) return true;
    }
  } else if (IsBinary(item, mem)) {
    char *item_data = BinaryData(item, mem);
    u32 item_len = BinaryLength(item, mem);
    i32 index = FindString(item_data, item_len, bin_data, bin_len);
    return index >= 0;
  }

  return false;
}

Val BinaryCat(Val binary1, Val binary2, Mem *mem)
{
  Val result;
  u32 len1, len2;
  char *data;
  len1 = BinaryLength(binary1, mem);
  len2 = BinaryLength(binary2, mem);
  result = MakeBinary(len1+len2, mem);
  data = BinaryData(result, mem);
  Copy(BinaryData(binary1, mem), data, len1);
  Copy(BinaryData(binary2, mem), data+len1, len2);
  return result;
}

/*
Maps are hash array mapped tries (HAMTs). A map node is a map header followed by
up to 16 slots. The header value is a bitmap of which slots are present. A slot
can contain a key/value pair, or a map pointer to a child node. Shoutout to Phil
Bagwell.
*/

#define NodeCount(header)       PopCount((header) & 0xFFFF)
#define SlotNum(hash, depth)    (((hash) >> (4 * (depth))) & 0x0F)
#define IndexNum(header, slot)  PopCount((header) & (Bit(slot) - 1))
#define NodeRef(node, i, mem)   VecRef(mem, RawVal(node) + 1 + (i))

/* This just creates a node with the given header, filling it with the
appropriate amount of slots. Between this and NodeRef, we isolate the memory
format of nodes */
static Val MakeMapNode(u32 header, Mem *mem)
{
  Val map = ObjVal(mem->count);
  u32 num_children = NodeCount(header);

  if (CapacityLeft(mem) < Max(2, num_children+1)) ResizeMem(mem, Max(2*mem->capacity, mem->count+num_children+1));
  Assert(CapacityLeft(mem) >= Max(2, num_children + 1));

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

u32 MapCount(Val map, Mem *mem)
{
  Val header = VecRef(mem, RawVal(map));
  u32 count = 0;
  u32 num_children = NodeCount(header);
  u32 i;
  for (i = 0; i < num_children; i++) {
    Val child = NodeRef(map, i, mem);
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
  Val header = VecRef(mem, RawVal(node));
  u32 hash = Hash(&key, sizeof(Val));
  u32 slot = SlotNum(hash, level);
  u32 index = IndexNum(header, slot);

  if ((header & Bit(slot)) == 0) return false;

  node = NodeRef(node, index, mem);
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

/* Creates an internal map node with two children. If they map to the same slot,
another internal node is created one level deeper. */
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
    NodeRef(node, 0, mem) = MakeInternalNode(child1, child2, level+1, mem);
  } else if (slot1 < slot2) {
    NodeRef(node, 0, mem) = child1;
    NodeRef(node, 1, mem) = child2;
  } else {
    NodeRef(node, 0, mem) = child2;
    NodeRef(node, 1, mem) = child1;
  }
  return node;
}

/* Copies a node with the child inserted in the given slot. If the child is nil,
the slot is emptied (and the node is reduced to a pair if possible). If the slot
is occupied, the old child is replaced. */
static Val NodeUpdate(Val node, u32 slot, Val child, Mem *mem)
{
  Val header = VecRef(mem, RawVal(node));
  u32 num_children = NodeCount(header);
  u32 index = IndexNum(header, slot);
  bool replace = (header & Bit(slot)) != 0;
  u32 i;
  Val new_node;

  if (child == Nil) {
    if (!replace) return node;
    if (num_children == 2) return NodeRef(node, !index, mem);
    new_node = MakeMapNode(MapHeader(header & ~Bit(slot)), mem);
    for (i = 0; i < num_children; i++) {
      NodeRef(new_node, i, mem) = NodeRef(node, i, mem);
    }
  } else {
    new_node = MakeMapNode(MapHeader(header | Bit(slot)), mem);
    for (i = 0; i < index; i++) {
      NodeRef(new_node, i, mem) = NodeRef(node, i, mem);
    }
    NodeRef(new_node, index, mem) = child;
    for (i = index+replace; i < num_children; i++) {
      NodeRef(new_node, i+1-replace, mem) = NodeRef(node, i, mem);
    }
  }

  return new_node;
}

static Val NodeSet(Val node, Val key, Val value, u32 level, Mem *mem)
{
  Val header = VecRef(mem, RawVal(node));
  u32 hash = Hash(&key, sizeof(Val));
  u32 slot = SlotNum(hash, level);
  u32 index = IndexNum(header, slot);

  if ((header & Bit(slot)) == 0) {
    /* empty slot, easy insert */
    Val pair = Pair(key, value, mem);
    return NodeUpdate(node, slot, pair, mem);
  } else {
    Val child = NodeRef(node, index, mem);
    if (!IsPair(child)) {
      /* slot is a sub-map - recurse down a level */
      Val new_child = NodeSet(child, key, value, level + 1, mem);
      return NodeUpdate(node, slot, new_child, mem);
    } else if (key == Head(child, mem)) {
      /* key match, easy insert */
      Val pair = Pair(key, value, mem);
      return NodeUpdate(node, slot, pair, mem);
    } else {
      /* collision with another leaf - create a new internal node */
      Val pair = Pair(key, value, mem);
      Val new_child = MakeInternalNode(child, pair, level + 1, mem);
      return NodeUpdate(node, slot, new_child, mem);
    }
  }
}

Val MapSet(Val map, Val key, Val value, Mem *mem)
{
  return NodeSet(map, key, value, 0, mem);
}

static Val NodeDelete(Val node, Val key, u32 level, Mem *mem)
{
  Val header = VecRef(mem, RawVal(node));
  u32 hash = Hash(&key, sizeof(Val));
  u32 slot = SlotNum(hash, level);
  u32 index = IndexNum(header, slot);
  Val child;

  if ((header & Bit(slot)) == 0) return node;

  child = NodeRef(node, index, mem);
  if (!IsPair(child)) {
    Val new_child = NodeDelete(child, key, level + 1, mem);
    if (child == new_child) return node;
    return NodeUpdate(node, slot, new_child, mem);
  } else if (key == Head(child, mem)) {
    return Nil;
  } else {
    return node;
  }
}

Val MapDelete(Val map, Val key, Mem *mem)
{
  return NodeDelete(map, key, 0, mem);
}

static bool NodeGet(Val node, Val key, u32 level, Mem *mem)
{
  Val header = VecRef(mem, RawVal(node));
  u32 hash = Hash(&key, sizeof(Val));
  u32 slot = SlotNum(hash, level);
  u32 index = IndexNum(header, slot);

  if ((header & Bit(slot)) == 0) return Nil;

  node = NodeRef(node, index, mem);
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

/* inserts all entries from map2 into map1, overwriting existing keys in map1 */
Val MapMerge(Val map1, Val map2, Mem *mem)
{
  u32 i;
  if (MapCount(map2, mem) == 0) return map1;
  if (MapCount(map1, mem) == 0) return map2;

  for (i = 0; i < MapCount(map2, mem); i++) {
    Val child = NodeRef(map2, i, mem);
    if (IsPair(child)) {
      map1 = MapSet(map1, Head(child, mem), Tail(child, mem), mem);
    } else {
      map1 = MapMerge(map1, child, mem);
    }
  }

  return map1;
}

Val MapKeys(Val map, Val keys, Mem *mem)
{
  Val header = VecRef(mem, RawVal(map));
  u32 num_children = NodeCount(header);
  u32 i;

  for (i = 0; i < num_children; i++) {
    Val node = NodeRef(map, i, mem);
    if (IsPair(node)) {
      keys = Pair(Head(node, mem), keys, mem);
    } else {
      keys = MapKeys(node, keys, mem);
    }
  }

  return keys;
}

Val MapValues(Val map, Val values, Mem *mem)
{
  Val header = VecRef(mem, RawVal(map));
  u32 num_children = NodeCount(header);
  u32 i;

  for (i = 0; i < num_children; i++) {
    Val node = NodeRef(map, i, mem);
    if (IsPair(node)) {
      values = Pair(Tail(node, mem), values, mem);
    } else {
      values = MapValues(node, values, mem);
    }
  }

  return values;
}

static bool MapIsSubset(Val v1, Val v2, Mem *mem)
{
  Val header = VecRef(mem, RawVal(v1));
  u32 size = NodeCount(header);
  u32 i;
  for (i = 0; i < size; i++) {
    Val node = NodeRef(v1, i, mem);
    if (IsPair(node)) {
      Val key = Head(node, mem);
      Val value = Tail(node, mem);
      if (!MapContains(v2, key, mem)) return false;
      if (!ValEqual(value, MapGet(v2, key, mem), mem)) return false;
    } else {
      if (!MapIsSubset(node, v2, mem)) return false;
    }
  }
  return true;
}

bool ValEqual(Val v1, Val v2, Mem *mem)
{
  if (IsNum(v1, mem) && IsNum(v2, mem)) {
    return RawNum(v1) == RawNum(v2);
  }

  if (TypeSym(v1, mem) != TypeSym(v2, mem)) return false;

  if (IsSym(v1)) return v1 == v2;
  if (v1 == Nil && v2 == Nil) return true;
  if (IsPair(v1)) {
    return ValEqual(Head(v1, mem), Head(v2, mem), mem)
        && ValEqual(Tail(v1, mem), Tail(v2, mem), mem);
  }
  if (IsTuple(v1, mem)) {
    u32 i;
    if (TupleLength(v1, mem) != TupleLength(v2, mem)) return false;
    for (i = 0; i < TupleLength(v1, mem); i++) {
      if (!ValEqual(TupleGet(v1, i, mem), TupleGet(v2, i, mem), mem)) return false;
    }
    return true;
  }
  if (IsBinary(v1, mem)) {
    u32 i;
    char *data1 = BinaryData(v1, mem);
    char *data2 = BinaryData(v2, mem);
    if (BinaryLength(v1, mem) != BinaryLength(v2, mem)) return false;
    for (i = 0; i < BinaryLength(v1, mem); i++) {
      if (data1[i] != data2[i]) return false;
    }
    return true;
  }
  if (IsMap(v1, mem)) {
    if (MapCount(v1, mem) != MapCount(v2, mem)) return false;
    return MapIsSubset(v1, v2, mem);
  }
  return false;
}

/* Cheney's algorithm */
static Val CopyValue(Val value, Mem *from, Mem *to);
static Val ValSize(Val value);
void CollectGarbage(Val *roots, u32 num_roots, Mem *mem)
{
  u32 i;
  Mem new_mem;

  InitMem(&new_mem, mem->capacity);

  for (i = 0; i < num_roots; i++) {
    roots[i] = CopyValue(roots[i], mem, &new_mem);
  }

  i = 2; /* Skip nil */
  while (i < new_mem.count) {
    Val value = VecRef(&new_mem, i);
    if (IsBinaryHeader(value)) {
      i += ValSize(value); /* skip over binary data, since they aren't values */
    } else {
      VecRef(&new_mem, i) = CopyValue(VecRef(&new_mem, i), mem, &new_mem);
      i++;
    }
  }

  Free(mem->items);
  *mem = new_mem;
}

static Val CopyValue(Val value, Mem *from, Mem *to)
{
  Val new_val = Nil;

  if (value == Nil) return Nil;
  if (!IsObj(value) && !IsPair(value)) return value;

  if (VecRef(from, RawVal(value)) == Moved) {
    return VecRef(from, RawVal(value)+1);
  }

  if (IsPair(value)) {
    new_val = Pair(Head(value, from), Tail(value, from), to);
  } else if (IsObj(value)) {
    u32 i;
    new_val = ObjVal(to->count);
    for (i = 0; i < ValSize(VecRef(from, RawVal(value))); i++) {
      PushMem(to, VecRef(from, RawVal(value)+i));
    }
  }

  VecRef(from, RawVal(value)) = Moved;
  VecRef(from, RawVal(value)+1) = new_val;
  return new_val;
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
