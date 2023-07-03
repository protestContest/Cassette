#pragma once
#include "mem.h"
#include "vm.h"

typedef struct {
  bool ok;
  u32 needs;
  u32 modifies;
  Val stmts;
} Seq;

#define Needs(seq, reg)     ((seq)->needs & (reg))
#define Modifies(seq, reg)  ((seq)->modifies & (reg))

typedef Val Linkage;
#define LinkReturn  SymbolFor("return")
#define LinkNext    SymbolFor("next")

#define MakeSeq(needs, modifies, stmts)   (Seq){true, needs, modifies, stmts}
#define EmptySeq(mem)   MakeSeq(0, 0, nil)
#define IsEmptySeq(seq) (IsNil((seq).stmts))

Seq AppendSeq(Seq seq1, Seq seq2, Mem *mem);
Seq TackOnSeq(Seq seq1, Seq seq2, Mem *mem);
Seq ParallelSeq(Seq seq1, Seq seq2, Mem *mem);
Seq Preserving(Reg regs, Seq seq1, Seq seq2, Mem *mem);
Seq EndWithLinkage(Seq seq, Linkage linkage, Mem *mem);
void PrintSeq(Seq seq, Mem *mem);
