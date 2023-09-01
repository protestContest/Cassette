#pragma once
#include "vm.h"
#include "seq.h"

#ifndef LIBCASSETTE
void PrintSeq(Seq seq, Heap *mem);
void DisassemblePart(Chunk *chunk, u32 start, u32 count);
void Disassemble(Chunk *chunk);
void PrintAST(Val ast, u32 indent, Heap *mem);
#endif