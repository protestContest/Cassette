#include "assemble.h"

static Val ReplaceReferences(Seq seq, Chunk *chunk, Heap *mem);
static u32 AssembleInstruction(Val stmts, Chunk *chunk, Heap *mem);
static HashMap LabelMap(Seq seq, Heap *mem);

void Assemble(Seq seq, Chunk *chunk, Heap *mem)
{
  Val stmts = ReplaceReferences(seq, chunk, mem);

  while (!IsNil(stmts)) {
    u32 len = AssembleInstruction(stmts, chunk, mem);
    stmts = TailList(stmts, len, mem);
  }
}

static Val ReplaceReferences(Seq seq, Chunk *chunk, Heap *mem)
{
  HashMap labels = LabelMap(seq, mem);
  u32 offset = 0;

  // handle first statement special cases
  while (true) {
    Val stmt = Head(seq.stmts, mem);
    if (IsTagged(stmt, "label", mem)) {
      seq.stmts = Tail(seq.stmts, mem);
    } else if (IsTagged(stmt, "source-ref", mem)) {
      HashMapSet(&chunk->source_map, offset, RawInt(Tail(stmt, mem)));
      seq.stmts = Tail(seq.stmts, mem);
    } else if (IsTagged(stmt, "source-file", mem)) {
      AddSourceFile(SymbolName(Tail(stmt, mem), mem), offset, chunk);
      seq.stmts = Tail(seq.stmts, mem);
    } else {
      break;
    }
  }

  Val stmts = seq.stmts;
  while (!IsNil(stmts)) {
    Val rest = Tail(stmts, mem);
    Val next = Head(rest, mem);

    if (IsTagged(next, "label", mem)) {
      // strip label markers
      SetTail(stmts, Tail(rest, mem), mem);
      continue;
    }

    if (IsTagged(next, "source-ref", mem)) {
      HashMapSet(&chunk->source_map, offset, RawInt(Tail(next, mem)));

      // strip source refs
      SetTail(stmts, Tail(rest, mem), mem);
      continue;
    }

    if (IsTagged(next, "source-file", mem)) {
      AddSourceFile(SymbolName(Tail(next, mem), mem), offset, chunk);

      // strip file refs
      SetTail(stmts, Tail(rest, mem), mem);
      continue;
    }

    Val stmt = Head(stmts, mem);
    if (IsTagged(stmt, "label-ref", mem)) {
      // replace label refs with relative offset
      u32 label = RawInt(Tail(stmt, mem));
      u32 location = HashMapGet(&labels, label);
      SetHead(stmts, IntVal((i32)location - (offset + 1)), mem);
    } else if (IsTagged(stmt, "module-ref", mem)) {
      // replace module refs with module name symbol
      Val name = Tail(stmt, mem);
      SetHead(stmts, name, mem);
    }

    offset++;
    stmts = Tail(stmts, mem);
  }

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
    } else if (!IsTagged(stmt, "source-ref", mem) && !IsTagged(stmt, "source-file", mem)) {
      offset++;
    }
    stmts = Tail(stmts, mem);
  }

  return labels;
}
