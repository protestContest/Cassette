#pragma once
#include "mem.h"
#include "chunk.h"
#include "seq.h"

Val MakeLabel(u32 label_num, Mem *mem);
Val Label(Val label, Mem *mem);
Val LabelRef(Val label, Mem *mem);
Seq LabelSeq(Val label, Mem *mem);
Val RegRef(u32 reg, Mem *mem);

Chunk Assemble(Seq stmts, Mem *mem);
