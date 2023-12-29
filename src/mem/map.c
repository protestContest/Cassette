#include "mem.h"
#include "univ/system.h"
#include "univ/hash.h"
#include "univ/math.h"

#define HeaderCount(header)     PopCount((header) & 0xFFFF)
#define NodeCount(node, mem)    HeaderCount(VecRef(mem, RawVal(node)))
#define SlotNum(hash, depth)    (((hash) >> (4 * (depth))) & 0x0F)
#define IndexNum(header, slot)  PopCount((header) & (Bit(slot) - 1))
#define NodeRef(node, i, mem)   VecRef(mem, RawVal(node) + 1 + (i))
#define NewNodeSize(header)     Max(2, (1 + HeaderCount(header)))

static bool NodeGet(Val node, Val key, u32 level, Mem *mem);
static u32 NodeSetSize(Val node, Val key, u32 level, Mem *mem);
static Val NodeSet(Val node, Val key, Val value, u32 level, Mem *mem);
static Val NodeDelete(Val node, Val key, u32 level, Mem *mem);
static bool NodeContains(Val node, Val key, u32 level, Mem *mem);
static u32 NewInternalNodeSize(Val key1, Val key2, u32 level, Mem *mem);
static Val NodeUpdate(Val node, u32 slot, Val child, Mem *mem);
static Val MakeInternalNode(Val child1, Val child2, u32 level, Mem *mem);
static Val MakeMapNode(u32 header, Mem *mem);

Val MakeMap(Mem *mem)
{
  Val map;

  if (!CheckMem(mem, 2)) CollectGarbage(mem);
  Assert(CheckMem(mem, 2));

  map = ObjVal(MemNext(mem));
  PushMem(mem, MapHeader(0));
  PushMem(mem, Nil);
  return map;
}

Val MapKeys(Val map, Val keys, Mem *mem)
{
  u32 num_keys = MapCount(map, mem);
  u32 num_children = NodeCount(map, mem);
  u32 i;

  if (!CheckMem(mem, 2*num_keys)) CollectGarbage(mem);

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
  u32 num_vals = MapCount(map, mem);
  u32 num_children = NodeCount(map, mem);
  u32 i;

  if (!CheckMem(mem, 2*num_vals)) CollectGarbage(mem);

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

Val MapGet(Val map, Val key, Mem *mem)
{
  return NodeGet(map, key, 0, mem);
}

Val MapGetKey(Val map, u32 index, Mem *mem)
{
  u32 num_children = NodeCount(map, mem);
  u32 i, count;

  count = 0;
  for (i = 0; i < num_children; i++) {
    Val node = NodeRef(map, i, mem);
    if (IsPair(node)) {
      if (count == index) return Head(node, mem);
      count++;
    } else {
      if (count + NodeCount(node, mem) > index) {
        return MapGetKey(node, index - count, mem);
      }
      count += NodeCount(node, mem);
    }
  }

  return Nil;
}

Val MapSetSize(Val map, Val key, Mem *mem)
{
  return NodeSetSize(map, key, 0, mem);
}

Val MapSet(Val map, Val key, Val value, Mem *mem)
{
  u32 needed = NodeSetSize(map, key, 0, mem);
  if (!CheckMem(mem, needed)) {
    PushRoot(mem, map);
    PushRoot(mem, key);
    PushRoot(mem, value);
    CollectGarbage(mem);
    value = PopRoot(mem);
    key = PopRoot(mem);
    map = PopRoot(mem);
  }
  Assert(CheckMem(mem, needed));
  return NodeSet(map, key, value, 0, mem);
}

Val MapDelete(Val map, Val key, Mem *mem)
{
  return NodeDelete(map, key, 0, mem);
}

u32 MapCount(Val map, Mem *mem)
{
  u32 count = 0;
  u32 num_children = NodeCount(map, mem);
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

bool MapContains(Val map, Val key, Mem *mem)
{
  return NodeContains(map, key, 0, mem);
}

bool MapIsSubset(Val v1, Val v2, Mem *mem)
{
  u32 size = NodeCount(v1, mem);
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

static u32 NodeSetSize(Val node, Val key, u32 level, Mem *mem)
{
  Val header = VecRef(mem, RawVal(node));
  u32 hash = Hash(&key, sizeof(Val));
  u32 slot = SlotNum(hash, level);
  u32 index = IndexNum(header, slot);

  if ((header & Bit(slot)) == 0) {
    return 2 + NewNodeSize(header | Bit(slot));
  } else {
    Val child = NodeRef(node, index, mem);
    if (!IsPair(child)) {
      return NodeSetSize(child, key, level + 1, mem) + NewNodeSize(header);
    } else if (key == Head(child, mem)) {
      return 2 + NewNodeSize(header);
    } else {
      return 2 + NewInternalNodeSize(Head(child, mem), key, level + 1, mem) + NewNodeSize(header);
    }
  }
}

static Val NodeSet(Val node, Val key, Val value, u32 level, Mem *mem)
{
  Val header = VecRef(mem, RawVal(node));
  u32 hash = Hash(&key, sizeof(Val));
  u32 slot = SlotNum(hash, level);
  u32 index = IndexNum(header, slot);
  Val pair, new_child;

  if ((header & Bit(slot)) == 0) {
    /* empty slot, easy insert */
    pair = Pair(key, value, mem);
    return NodeUpdate(node, slot, pair, mem);
  } else {
    Val child = NodeRef(node, index, mem);
    if (!IsPair(child)) {
      /* slot is a sub-map - recurse down a level */
      PushRoot(mem, node);
      new_child = NodeSet(child, key, value, level + 1, mem);
      node = PopRoot(mem);
      return NodeUpdate(node, slot, new_child, mem);
    } else if (key == Head(child, mem)) {
      /* key match, easy insert */
      pair = Pair(key, value, mem);
      return NodeUpdate(node, slot, pair, mem);
    } else {
      /* collision with another leaf - create a new internal node */
      pair = Pair(key, value, mem);
      PushRoot(mem, node);
      new_child = MakeInternalNode(child, pair, level + 1, mem);
      node = PopRoot(mem);
      return NodeUpdate(node, slot, new_child, mem);
    }
  }
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

static u32 NewInternalNodeSize(Val key1, Val key2, u32 level, Mem *mem)
{
  u32 hash1 = Hash(&key1, sizeof(Val));
  u32 slot1 = SlotNum(hash1, level);
  u32 hash2 = Hash(&key2, sizeof(Val));
  u32 slot2 = SlotNum(hash2, level);
  if (slot1 == slot2) {
    return NewNodeSize(1) + NewInternalNodeSize(key1, key2, level + 1, mem);
  } else {
    return NewNodeSize(Bit(slot1) | Bit(slot2));
  }
}

/* Copies a node with the child inserted in the given slot. If the child is nil,
the slot is emptied (and the node is reduced to a pair if possible). If the slot
is occupied, the old child is replaced. */
static Val NodeUpdate(Val node, u32 slot, Val child, Mem *mem)
{
  Val header = VecRef(mem, RawVal(node));
  u32 num_children = NodeCount(node, mem);
  u32 index = IndexNum(header, slot);
  bool replace = (header & Bit(slot)) != 0;
  u32 i;
  Val new_node;

  if (child == Nil) {
    if (!replace) return node;
    if (num_children == 2) return NodeRef(node, !index, mem);
    PushRoot(mem, node);
    new_node = MakeMapNode(MapHeader(header & ~Bit(slot)), mem);
    node = PopRoot(mem);
    for (i = 0; i < num_children; i++) {
      NodeRef(new_node, i, mem) = NodeRef(node, i, mem);
    }
  } else {
    PushRoot(mem, node);
    PushRoot(mem, child);
    new_node = MakeMapNode(MapHeader(header | Bit(slot)), mem);
    child = PopRoot(mem);
    node = PopRoot(mem);

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
  Val node;

  PushRoot(mem, child1);
  PushRoot(mem, child2);
  node = MakeMapNode(MapHeader(Bit(slot1) | Bit(slot2)), mem);
  child2 = PopRoot(mem);
  child1 = PopRoot(mem);

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

static Val MakeMapNode(u32 header, Mem *mem)
{
  Val node;
  u32 num_children = Max(1, HeaderCount(header));
  u32 i;

  if (!CheckMem(mem, num_children+1)) CollectGarbage(mem);
  Assert(CheckMem(mem, num_children+1));

  node = ObjVal(MemNext(mem));
  PushMem(mem, header);
  for (i = 0; i < num_children; i++) PushMem(mem, Nil);
  return node;
}
