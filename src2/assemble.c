#include "assemble.h"
#include "ops.h"
#include "vm.h"

Val MakeLabel(Mem *mem)
{
  static u32 label_num = 0;
  char label[12] = "lbl";
  u32 len = IntToString(label_num++, label+3, 7);
  return MakeSymbol(label, len+3, mem);
}

Val Label(Val label, Mem *mem)
{
  return Pair(SymbolFor("label"), label, mem);
}

Val LabelRef(Val label, Mem *mem)
{
  return Pair(SymbolFor("label-ref"), label, mem);
}

Seq LabelSeq(Val label, Mem *mem)
{
  return MakeSeq(0, 0,
    Pair(Label(label, mem), nil, mem));
}

Val RegRef(u32 reg, Mem *mem)
{
  Assert(reg != 0);
  return Pair(MakeSymbol("reg-ref", 7, mem), IntVal(reg), mem);
}

static u32 AssembleInstruction(Val stmts, Chunk *chunk, Mem *mem)
{
  OpCode op = OpFor(Head(stmts, mem));

  PushByte(chunk, op);
  for (u32 i = 0; i < OpLength(op) - 1; i++) {
    stmts = Tail(stmts, mem);

    Val arg = Head(stmts, mem);
    switch (OpArgType(op)) {
      case ArgsNone:
        break;
      case ArgsConst:
        if (IsSym(arg)) AddSymbol(chunk, arg, mem);
        PushByte(chunk, PushConst(chunk, arg));
        break;
      case ArgsReg:
        Assert(IsTagged(arg, "reg-ref", mem));
        PushByte(chunk, RawInt(Tail(arg, mem)));
        break;
    }
  }

  return OpLength(op);
}

static Map LabelMap(Seq seq, Mem *mem)
{
  Map labels;
  InitMap(&labels);

  u32 i = 0;
  Val stmts = seq.stmts;
  while (!IsNil(stmts)) {
    Val stmt = Head(stmts, mem);
    if (IsTagged(stmt, "label", mem)) {
      Val label = Tail(stmt, mem);
      MapSet(&labels, label.as_i, i);
    } else {
      i++;
    }
    stmts = Tail(stmts, mem);
  }

  return labels;
}

static Val ReplaceLabels(Seq seq, Mem *mem)
{
  Map labels = LabelMap(seq, mem);

  // skip any leading labels
  while (IsTagged(Head(seq.stmts, mem), "label", mem)) seq.stmts = Tail(seq.stmts, mem);

  Val stmts = seq.stmts;
  // replace ("label-ref", some-label) pairs with their locations
  while (!IsNil(stmts)) {
    Val stmt = Head(stmts, mem);
    if (IsTagged(stmt, "label-ref", mem)) {
      Val label = Tail(stmt, mem);
      Assert(MapContains(&labels, label.as_i));
      SetHead(stmts, IntVal(MapGet(&labels, label.as_i)), mem);
    }

    Val next_stmts = Tail(stmts, mem);
    while (IsTagged(Head(next_stmts, mem), "label", mem)) {
      next_stmts = Tail(next_stmts, mem);
    }
    SetTail(stmts, next_stmts, mem);
    stmts = next_stmts;
  }

  DestroyMap(&labels);
  return seq.stmts;
}

Chunk Assemble(Seq seq, Mem *mem)
{
  Chunk chunk;
  InitChunk(&chunk);

  Val stmts = ReplaceLabels(seq, mem);
  while (!IsNil(stmts)) {
    u32 len = AssembleInstruction(stmts, &chunk, mem);
    stmts = ListFrom(stmts, len, mem);
  }

  return chunk;
}
