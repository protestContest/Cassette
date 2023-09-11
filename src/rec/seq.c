#include "seq.h"
#include "debug.h"
#include "vm.h"

Val MakeLabel(void)
{
  static u32 label_num = 0;
  return IntVal(label_num++);
}

Val Label(Val label, Heap *mem)
{
  return Pair(MakeSymbol("label", mem), label, mem);
}

Val LabelRef(Val label, Heap *mem)
{
  return Pair(MakeSymbol("label-ref", mem), label, mem);
}

Seq LabelSeq(Val label, Heap *mem)
{
  return MakeSeq(0, 0,
    Pair(Label(label, mem), nil, mem));
}

Val ModuleRef(Val name, Heap *mem)
{
  return Pair(MakeSymbol("module-ref", mem), name, mem);
}

Seq ModuleSeq(char *file, Heap *mem)
{
  return MakeSeq(0, 0, Pair(Pair(MakeSymbol("source-file", mem), MakeSymbol(file, mem), mem), nil, mem));
}

Val SourceRef(u32 pos, Heap *mem)
{
  return Pair(MakeSymbol("source-ref", mem), IntVal(pos), mem);
}

Seq SourceSeq(u32 pos, Heap *mem)
{
  return MakeSeq(0, 0, Pair(SourceRef(pos, mem), nil, mem));
}

Seq AppendSeq(Seq seq1, Seq seq2, Heap *mem)
{
  if (IsEmptySeq(seq1)) return seq2;
  if (IsEmptySeq(seq2)) return seq1;

  // this sequence needs what seq1 needs, plus anything seq2 needs that seq1 doesn't modify
  u32 needs = seq1.needs | (seq2.needs & ~seq1.modifies);
  u32 modifies = seq1.modifies | seq2.modifies;
  ListConcat(seq1.stmts, seq2.stmts, mem);
  return MakeSeq(needs, modifies, seq1.stmts);
}

Seq TackOnSeq(Seq seq1, Seq seq2, Heap *mem)
{
  ListConcat(seq1.stmts, seq2.stmts, mem);
  return MakeSeq(seq1.needs, seq1.modifies, seq1.stmts);
}

Seq ParallelSeq(Seq seq1, Seq seq2, Heap *mem)
{
  u32 needs = seq1.needs | seq2.needs;
  u32 modifies = seq1.modifies | seq2.modifies;
  ListConcat(seq1.stmts, seq2.stmts, mem);
  return MakeSeq(needs, modifies, seq1.stmts);
}

Seq Preserving(u32 regs, Seq seq1, Seq seq2, Heap *mem)
{
  if ((regs & RCont) && Modifies(seq1, RCont) && Needs(seq2, RCont)) {
    seq1.needs |= RCont;
    seq1.modifies &= ~RCont;
    seq1.stmts = Pair(IntVal(OpSaveCont), seq1.stmts, mem);
    ListConcat(seq1.stmts, Pair(IntVal(OpRestCont), nil, mem), mem);
  }

  if ((regs & REnv) && Modifies(seq1, REnv) && Needs(seq2, REnv)) {
    seq1.needs |= REnv;
    seq1.modifies &= ~REnv;
    seq1.stmts = Pair(IntVal(OpSaveEnv), seq1.stmts, mem);
    ListConcat(seq1.stmts, Pair(IntVal(OpRestEnv), nil, mem), mem);
  }

  return AppendSeq(seq1, seq2, mem);
}

static Seq CompileLinkage(Linkage linkage, Heap *mem)
{
  if (Eq(linkage, LinkNext)) return EmptySeq();

  if (Eq(linkage, LinkReturn)) {
    return MakeSeq(RCont, 0, Pair(IntVal(OpReturn), nil, mem));
  }

  return MakeSeq(0, 0,
    Pair(IntVal(OpJump),
    Pair(LabelRef(linkage, mem), nil, mem), mem));
}

Seq EndWithLinkage(Linkage linkage, Seq seq, Heap *mem)
{
  return Preserving(RCont, seq, CompileLinkage(linkage, mem), mem);
}
