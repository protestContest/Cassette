#include "assemble.h"
#include "ops.h"
#include "vm.h"

Seq AppendSeq(Seq seq1, Seq seq2, Mem *mem)
{
  if (IsEmptySeq(seq1)) return seq2;
  if (IsEmptySeq(seq2)) return seq1;

  // this sequence needs what seq1 needs, plus anything seq2 needs that seq1 doesn't modify
  u32 needs = seq1.needs | (seq2.needs & ~seq1.modifies);
  u32 modifies = seq1.modifies | seq2.modifies;
  return MakeSeq(needs, modifies, ListConcat(seq1.stmts, seq2.stmts, mem));
}

Seq TackOnSeq(Seq seq1, Seq seq2, Mem *mem)
{
  return MakeSeq(seq1.needs, seq1.modifies, ListConcat(seq1.stmts, seq2.stmts, mem));
}

Seq ParallelSeq(Seq seq1, Seq seq2, Mem *mem)
{
  u32 needs = seq1.needs | seq2.needs;
  u32 modifies = seq1.modifies | seq2.modifies;
  return MakeSeq(needs, modifies, ListConcat(seq1.stmts, seq2.stmts, mem));
}

Val NextLabel(u32 label_num, Mem *mem)
{
  char label[12] = "lbl";
  u32 len = IntToString(label_num, label+3, 7);
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
  return Pair(SymbolFor("reg"), IntVal(reg), mem);
}

static void PrintReg(Reg reg)
{
  switch (reg) {
  case RegCont:
    Print("[con]");
    break;
  case RegEnv:
    Print("[env]");
    break;
  }
}

static u32 PrintStmt(Val stmts, Mem *mem)
{
  OpCode op = OpFor(Head(stmts, mem));
  Print(OpName(op));
  stmts = Tail(stmts, mem);

  for (u32 i = 0; i < OpLength(op) - 1; i++) {
    Val arg = Head(stmts, mem);
    Print(" ");

    if (IsSym(arg)) {
      Print(SymbolName(arg, mem));
    } else if (IsTagged(arg, "label-ref", mem)) {
      Print(":");
      Print(SymbolName(Tail(arg, mem), mem));
    } else if (IsTagged(arg, "reg", mem)) {
      PrintReg(RawInt(Tail(arg, mem)));
    } else {
      DebugVal(arg, mem);
    }

    stmts = Tail(stmts, mem);
  }
  Print("\n");
  return OpLength(op);
}

void PrintSeq(Seq seq, Mem *mem)
{
  if (seq.needs > 0) {
    Print("Needs ");
    for (u32 i = 0; i < NUM_REGS; i++) {
      if (Needs(&seq, 1 << i)) {
        PrintReg(1 << i);
        Print(" ");
      }
    }
  }

  if (seq.modifies > 0) {
    Print("Modifies ");
    for (u32 i = 0; i < NUM_REGS; i++) {
      if (Modifies(&seq, 1 << i)) {
        PrintReg(1 << i);
        Print(" ");
      }
    }
  }

  if (seq.needs > 0 || seq.modifies > 0) Print("\n");


  i32 i = 0;
  Val stmts = seq.stmts;
  while (!IsNil(stmts)) {
    Val stmt = Head(stmts, mem);

    if (IsTagged(stmt, "label", mem)) {
      Val label = Tail(stmt, mem);
      Print(SymbolName(label, mem));
      Print("\n");
      i++;
      stmts = Tail(stmts, mem);
    } else {
      PrintIntN(i, 4, ' ');
      Print("│ ");

      u32 len = PrintStmt(stmts, mem);
      i += len;
      stmts = ListFrom(stmts, len, mem);
    }
  }
}

static u32 AssembleInstruction(Val stmts, Chunk *chunk, Mem *mem)
{
  OpCode op = RawInt(Head(stmts, mem));

  PushByte(chunk, op);
  for (u32 i = 0; i < OpLength(op); i++) {
    stmts = Tail(stmts, mem);
    Print(" ");

    Val arg = Head(stmts, mem);
    switch (OpArgType(op)) {
      case ArgsNone:
        break;
      case ArgsConst:
        PushByte(chunk, PushConst(chunk, arg));
        break;
      case ArgsReg:
        Assert(IsTagged(arg, "reg-ref", mem));
        PushByte(chunk, RawInt(Tail(arg, mem)));
        break;
      case ArgsLength:
        PushByte(chunk, RawInt(arg));
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

  Val stmts = seq.stmts;
  // skip any leading labels
  while (IsTagged(Head(stmts, mem), "label", mem)) stmts = Tail(stmts, mem);
  seq.stmts = stmts;

  // replace ("label-ref", some-label) pairs with their locations
  while (!IsNil(stmts)) {
    Val stmt = Head(stmts, mem);
    if (IsTagged(stmt, "label-ref", mem)) {
      Val label = Tail(stmt, mem);
      Assert(MapContains(&labels, label.as_i));
      SetHead(stmt, IntVal(MapGet(&labels, label.as_i)), mem);
    }

    stmts = Tail(stmts, mem);
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
