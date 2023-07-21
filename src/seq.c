#include "seq.h"
#include "ops.h"
#include "assemble.h"
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

Seq Preserving(Reg regs, Seq seq1, Seq seq2, Mem *mem)
{
  for (u32 i = 0; i < NUM_REGS; i++) {
    Reg reg = (1 << i);
    if ((regs & reg) == 0) continue;

    if (Needs(&seq2, reg) && Modifies(&seq1, reg)) {
      seq1.needs |= reg;
      seq1.modifies &= ~reg;
      seq1.stmts =
        ListConcat(
          Pair(OpSymbol(OpSave),
          Pair(RegRef(reg, mem),
            seq1.stmts, mem), mem),
          Pair(OpSymbol(OpRestore),
          Pair(RegRef(reg, mem), nil, mem), mem), mem);
    }
  }

  return AppendSeq(seq1, seq2, mem);
}

static Seq CompileLinkage(Linkage linkage, Mem *mem)
{
  if (Eq(linkage, LinkNext)) return EmptySeq();

  if (Eq(linkage, LinkReturn)) {
    return MakeSeq(RegCont, 0, Pair(OpSymbol(OpReturn), nil, mem));
  }

  return MakeSeq(0, 0,
    Pair(OpSymbol(OpJump),
    Pair(LabelRef(linkage, mem), nil, mem), mem));
}

Seq EndWithLinkage(Linkage linkage, Seq seq, Mem *mem)
{
  return Preserving(RegCont, seq, CompileLinkage(linkage, mem), mem);
}

static void PrintReg(Reg reg)
{
  Print(RegName(reg));
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
    } else if (IsTagged(arg, "reg-ref", mem)) {
      PrintReg(RawInt(Tail(arg, mem)));
    } else {
      InspectVal(arg, mem);
    }

    stmts = Tail(stmts, mem);
  }
  Print("\n");
  return OpLength(op);
}

void PrintSeq(Seq seq, Mem *mem)
{
  Print("┌───");
  if (seq.needs > 0 || seq.modifies > 0) {
    Print(" ");
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

  }
  Print("\n");

  Val stmts = seq.stmts;
  while (!IsNil(stmts)) {
    Print("│ ");
    Val stmt = Head(stmts, mem);

    if (IsTagged(stmt, "label", mem)) {
      Val label = Tail(stmt, mem);
      Print(SymbolName(label, mem));
      Print(":\n");
      stmts = Tail(stmts, mem);
    } else {
      Print("  ");
      u32 len = PrintStmt(stmts, mem);
      stmts = ListFrom(stmts, len, mem);
    }
  }

  Print("└───\n");
}
