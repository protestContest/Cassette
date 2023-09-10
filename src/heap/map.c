#include "map.h"
#include "tuple.h"
#include "list.h"
#include "binary.h"
#include "symbol.h"
#include "debug.h"

#define slotBits            4
#define slotMask            ((1 << slotBits) - 1)
#define SlotFromKey(k, d)   ((RawVal(k) >> ((d) * slotBits)) & slotMask)
#define ChildIndex(bm, s)   PopCount((bm) & Bit(s) - 1)
#define NodeBitmap(n, m)    RawVal((m)->values[RawVal(n)])
#define NodeChildren(n, m)  ((m)->values[RawVal(n) + 1])
#define LeafKey(n, m)       ((m)->values[RawVal(n) + 1])
#define LeafContents(n, m)  ((m)->values[RawVal(n) + 2])
#define IsLeaf(n, m)        (NodeBitmap(n, m) == 0)

/*
Map node structure:

Internal nodes:
- Map header: bitmap of active branches
- Tuple of children

Leaf nodes:
- Map header: zero bitmap (no branches)
- Key (a symbol value, which is a hash)
- Value

Empty map:
- Map header: zero bitmap (a leaf)
- nil key
- nil value
*/

static Val MakeNode(u32 branch_bitmap, Val children, Heap *mem)
{
  Val node = ObjVal(MemSize(mem));
  VecPush(mem->values, MapHeader(branch_bitmap));
  VecPush(mem->values, children);
  return node;
}

static Val MakeLeaf(Val key, Val value, Heap *mem)
{
  Val leaf = ObjVal(MemSize(mem));
  VecPush(mem->values, MapHeader(0));
  VecPush(mem->values, key);
  VecPush(mem->values, value);

  return leaf;
}

static Val PutNodeChild(Val node, u32 slot, Val child, Heap *mem)
{
  // copies a node and sets slot to given child

  u32 bitmap = NodeBitmap(node, mem);
  Val children;
  if (bitmap & Bit(slot)) {
    // replace child
    u32 index = ChildIndex(bitmap, slot);
    children = TuplePut(NodeChildren(node, mem), index, child, mem);
  } else {
    bitmap |= Bit(slot);
    u32 index = ChildIndex(bitmap, slot);
    children = TupleInsert(NodeChildren(node, mem), index, child, mem);
  }

  return MakeNode(bitmap, children, mem);
}

static Val PutNode(Val node, u32 depth, Val key, Val value, Heap *mem);

static Val PutLeaf(Val node, u32 depth, Val key, Val value, Heap *mem)
{
  if (IsNil(LeafKey(node, mem))) {
    // empty map, return a new one
    return MakeLeaf(key, value, mem);
  } else if (Eq(key, LeafKey(node, mem))) {
    if (Eq(value, LeafContents(node, mem))) {
      // no-op
      return node;
    } else {
      // replace leaf
      return MakeLeaf(key, value, mem);
    }
  } else {
    // split leaf
    u32 old_index = SlotFromKey(LeafKey(node, mem), depth);
    Val children = MakeTuple(1, mem);
    TupleSet(children, 0, node, mem);
    Val new_node = MakeNode(Bit(old_index), children, mem);
    return PutNode(new_node, depth, key, value, mem);
  }
}

static Val PutNode(Val node, u32 depth, Val key, Val value, Heap *mem)
{
  if (IsLeaf(node, mem)) {
    return PutLeaf(node, depth, key, value, mem);
  }

  u32 bitmap = NodeBitmap(node, mem);
  u32 slot = SlotFromKey(key, depth);
  if (bitmap & Bit(slot)) {
    // slot taken; try to insert into that node
    u32 index = ChildIndex(bitmap, slot);
    Val child = TupleGet(NodeChildren(node, mem), index, mem);

    // create replacement sub-tree
    Val new_child = PutNode(child, depth+1, key, value, mem);
    if (Eq(child, new_child)) return node;

    // clone this node and set it to the new child
    return PutNodeChild(node, slot, new_child, mem);
  } else {
    // slot free; clone this node and set it
    Val leaf = MakeLeaf(key, value, mem);
    return PutNodeChild(node, slot, leaf, mem);
  }
}

Val MakeMap(Heap *mem)
{
  Val map = ObjVal(MemSize(mem));
  VecPush(mem->values, MapHeader(0));
  VecPush(mem->values, nil);
  return map;
}

bool IsMap(Val val, Heap *mem)
{
  return IsObj(val) && IsMapHeader(mem->values[RawVal(val)]);
}

u32 MapCount(Val map, Heap *mem)
{
  if (IsLeaf(map, mem)) {
    if (IsNil(LeafKey(map, mem))) return 0;
    return 1;
  }

  Val children = NodeChildren(map, mem);
  u32 count = 0;
  for (u32 i = 0; i < TupleLength(children, mem); i++) {
    count += MapCount(TupleGet(children, i,  mem), mem);
  }

  return count;
}

static Val GetLeaf(Val node, u32 depth, Val key, Heap *mem)
{
  if (IsLeaf(node, mem)) {
    if (Eq(key, LeafKey(node, mem))) {
      return node;
    } else {
      // something else with the same prefix; our guy isn't in here
      return nil;
    }
  }

  u32 bitmap = NodeBitmap(node, mem);
  u32 slot = SlotFromKey(key, depth);

  if (bitmap & Bit(slot)) {
    u32 index = ChildIndex(bitmap, slot);
    Val children = NodeChildren(node, mem);
    Val child = TupleGet(children, index, mem);
    return GetLeaf(child, depth+1, key, mem);
  } else {
    // this node doesn't have a child branch for us; our guy isn't in here
    return nil;
  }
}

bool MapContains(Val map, Val key, Heap *mem)
{
  return !Eq(MapGet(map, key, mem), Undefined);
}

Val MapGet(Val map, Val key, Heap *mem)
{
  Val leaf = GetLeaf(map, 0, key, mem);
  if (IsNil(leaf)) return Undefined;

  return LeafContents(leaf, mem);
}

Val MapPut(Val map, Val key, Val value, Heap *mem)
{
  return PutNode(map, 0, key, value, mem);
}

Val MapDelete(Val map, Val key, Heap *mem)
{
  return Error;
}

Val MapFrom(Val keys, Val vals, Heap *mem)
{
  Val map = MakeMap(mem);

  for (u32 i = 0; i < TupleLength(keys, mem); i++) {
    Val key = TupleGet(keys, i, mem);
    Val val = TupleGet(vals, i, mem);
    map = MapPut(map, key, val, mem);
  }

  return map;
}

static u32 CollectKeys(Val map, Val keys, u32 index, Heap *mem)
{
  if (IsLeaf(map, mem)) {
    if (IsNil(LeafKey(map, mem))) return 0;
    TupleSet(keys, index, LeafKey(map, mem), mem);
    return 1;
  }

  Val children = NodeChildren(map, mem);
  u32 count = 0;
  for (u32 i = 0; i < TupleLength(children, mem); i++) {
    count += CollectKeys(TupleGet(children, i, mem), keys, index + count, mem);
  }

  return count;
}

static u32 CollectValues(Val map, Val values, u32 index, Heap *mem)
{
  if (IsLeaf(map, mem)) {
    if (IsNil(LeafKey(map, mem))) return 0;
    TupleSet(values, index, LeafContents(map, mem), mem);
    return 1;
  }

  Val children = NodeChildren(map, mem);
  u32 count = 0;
  for (u32 i = 0; i < TupleLength(children, mem); i++) {
    count += CollectValues(TupleGet(children, i, mem), values, index + count, mem);
  }

  return count;
}

Val MapKeys(Val map, Heap *mem)
{
  u32 length = MapCount(map, mem);

  Val keys = MakeTuple(length, mem);
  CollectKeys(map, keys, 0, mem);
  return keys;
}

Val MapValues(Val map, Heap *mem)
{
  u32 length = MapCount(map, mem);
  Val values = MakeTuple(length, mem);
  CollectValues(map, values, 0, mem);
  return values;
}

u32 InspectNode(Val node, u32 depth, Heap *mem)
{
  Print("[");
  PrintInt(depth);
  Print("]");
  PrintHexN(NodeBitmap(node, mem), 4);

  if (IsLeaf(node, mem)) {
    Print(" ");
    Inspect(LeafKey(node, mem), mem);
    if (!IsNil(LeafKey(node, mem))) {
      Print(" (");
      PrintHexN(RawVal(HashValue(LeafKey(node, mem), mem)), 5);
      Print(")");
      Print(" => ");
      DebugVal(LeafContents(node, mem), mem);
    }
  } else {
    Print(" #");
    Val children = NodeChildren(node, mem);
    PrintInt(TupleLength(children, mem));
    for (u32 i = 0; i < TupleLength(children, mem); i++) {
      Print("\n");
      for (u32 j = 0; j < depth + 1; j++) Print("  ");
      InspectNode(TupleGet(children, i, mem), depth + 1, mem);
    }
  }

  // if (IsLeaf(map, mem)) {
  //   Val key = LeafKey(map, mem);
  //   if (IsNil(key)) return 0;
  //   Val val = LeafContents(map, mem);
  //   u32 len = Print(SymbolName(key, mem));
  //   len += Print(": ");
  //   len += Inspect(val, mem);
  //   return len;
  // }

  // Val children = NodeChildren(map, mem);
  // u32 num = TupleLength(children, mem);
  // u32 len = 0;
  // for (u32 i = 0; i < num; i++) {
  //   len += InspectMap(TupleGet(children, i, mem), mem);
  //   if (i < num-1) len += Print(", ");
  // }
  return 0;
}

u32 InspectMap(Val map, Heap *mem)
{
  InspectNode(map, 0, mem);
  Print("\n");
  return 0;
}
