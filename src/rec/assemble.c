#include "assemble.h"

static Val ReplaceReferences(Seq seq, Heap *mem);
static u32 AssembleInstruction(Val stmts, Chunk *chunk, Heap *mem);
static HashMap LabelMap(Seq seq, Heap *mem);
static HashMap ModuleMap(Seq seq, Heap *mem);

void Assemble(Seq seq, Chunk *chunk, Heap *mem)
{
  Val stmts = ReplaceReferences(seq, mem);
  while (!IsNil(stmts)) {
    u32 len = AssembleInstruction(stmts, chunk, mem);
    stmts = ListAfter(stmts, len, mem);
  }
}

static Val ReplaceReferences(Seq seq, Heap *mem)
{
  HashMap labels = LabelMap(seq, mem);
  HashMap modules = ModuleMap(seq, mem);

  // skip any leading labels
  while (IsTagged(Head(seq.stmts, mem), "label", mem)) {
    seq.stmts = Tail(seq.stmts, mem);
  }

  Val stmts = seq.stmts;
  u32 offset = 0;
  // replace ("label-ref", some-label) pairs with their relative locations
  while (!IsNil(stmts)) {
    // strip label statements
    Val next = Tail(stmts, mem);
    while (IsTagged(Head(next, mem), "label", mem)) {
      next = Tail(next, mem);
    }
    SetTail(stmts, next, mem);

    Val stmt = Head(stmts, mem);
    if (IsTagged(stmt, "label-ref", mem)) {
      u32 label = RawInt(Tail(stmt, mem));
      u32 location = HashMapGet(&labels, label);
      SetHead(stmts, IntVal((i32)location - (offset + 1)), mem);
    } else if (IsTagged(stmt, "module-ref", mem) || IsTagged(stmt, "module", mem)) {
      Val name = Tail(stmt, mem);
      HashMapSet(&modules, name.as_i, 1);
      SetHead(stmts, name, mem);
    }

    offset++;
    stmts = Tail(stmts, mem);
  }

  DestroyHashMap(&modules);
  DestroyHashMap(&labels);
  return seq.stmts;
}

static u32 AssembleInstruction(Val stmts, Chunk *chunk, Heap *mem)
{
  OpCode op = RawInt(Head(stmts, mem));

  PushByte(op, chunk);
  for (u32 i = 0; i < OpLength(op) - 1; i++) {
    stmts = Tail(stmts, mem);
    Val arg = Head(stmts, mem);
    PushConst(arg, chunk);
    if (IsSym(arg)) {
      MakeSymbol(SymbolName(arg, mem), &chunk->constants);
    }
  }

  return OpLength(op);
}

static HashMap LabelMap(Seq seq, Heap *mem)
{
  HashMap labels = EmptyHashMap;

  Val stmts = seq.stmts;
  u32 offset = 0;
  while (!IsNil(stmts)) {
    Val stmt = Head(stmts, mem);
    if (IsTagged(stmt, "label", mem)) {
      u32 label = RawInt(Tail(stmt, mem));
      HashMapSet(&labels, label, offset);
    } else {
      offset++;
    }
    stmts = Tail(stmts, mem);
  }

  return labels;
}

static HashMap ModuleMap(Seq seq, Heap *mem)
{
  HashMap mods = EmptyHashMap;

  Val stmts = seq.stmts;
  u32 num_mods = 0;
  while (!IsNil(stmts)) {
    Val stmt = Head(stmts, mem);
    if (IsTagged(stmt, "module", mem)) {
      Val name = Tail(stmt, mem);
      HashMapSet(&mods, name.as_i, num_mods);
      num_mods++;
    }
    stmts = Tail(stmts, mem);
  }

  return mods;
}
