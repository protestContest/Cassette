#pragma once
#include "mem.h"
#include "chunk.h"

typedef struct {
  bool ok;
  u32 needs;
  u32 modifies;
  Val stmts;
} Seq;

#define NUM_REGS 2

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

Val NextLabel(u32 label_num, Mem *mem);
Val Label(Val label, Mem *mem);
Val LabelRef(Val label, Mem *mem);
Seq LabelSeq(Val label, Mem *mem);

Chunk Assemble(Seq stmts, Mem *mem);
