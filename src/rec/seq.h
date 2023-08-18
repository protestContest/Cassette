#pragma once
#include "heap.h"

typedef struct {
  bool ok;
  u32 needs;
  u32 modifies;
  Val stmts;
} Seq;

#define REnv  (1 << RegEnv)
#define RCont (1 << RegCont)

#define Needs(seq, reg)     ((seq).needs & (reg))
#define Modifies(seq, reg)  ((seq).modifies & (reg))

typedef Val Linkage;
#define LinkReturn  SymbolFor("return")
#define LinkExport  SymbolFor("export")
#define LinkNext    SymbolFor("next")

#define MakeSeq(needs, modifies, stmts)   (Seq){true, needs, modifies, stmts}
#define EmptySeq(mem)   MakeSeq(0, 0, nil)
#define IsEmptySeq(seq) (IsNil((seq).stmts))

Val MakeLabel(void);
char *GetLabel(Val label);
Val Label(Val label, Heap *mem);
Val LabelRef(Val label, Heap *mem);
Seq LabelSeq(Val label, Heap *mem);

Seq AppendSeq(Seq seq1, Seq seq2, Heap *mem);
Seq TackOnSeq(Seq seq1, Seq seq2, Heap *mem);
Seq ParallelSeq(Seq seq1, Seq seq2, Heap *mem);
Seq Preserving(u32 regs, Seq seq1, Seq seq2, Heap *mem);
Seq EndWithLinkage(Linkage linkage, Seq seq, Heap *mem);
