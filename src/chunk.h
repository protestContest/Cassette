#pragma once
#include "mem.h"


typedef struct {
  u8 *data;
  Val *constants;
} Chunk;

u8 ChunkRef(Chunk *chunk, u32 index);
Val ChunkConst(Chunk *chunk, u32 index);

Chunk Assemble(Val stmts, Mem *mem);
void Disassemble(Chunk *chunk, Mem *mem);
u32 PrintInstruction(Chunk *chunk, u32 i, Mem *mem);
void PrintChunkConstants(Chunk *chunk, Mem *mem);
