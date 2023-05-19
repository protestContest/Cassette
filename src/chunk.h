#pragma once
#include "mem.h"
#include "string_table.h"

typedef struct {
  u8 *code;
  Val *constants;
  StringTable symbols;
} Chunk;

void InitChunk(Chunk *chunk);

u8 ChunkRef(Chunk *chunk, u32 index);
Val ChunkConst(Chunk *chunk, u32 index);

Chunk Assemble(Val stmts, Mem *mem);
void Disassemble(Chunk *chunk, Mem *mem);
u32 PrintInstruction(Chunk *chunk, u32 i, Mem *mem);
void PrintChunkConstants(Chunk *chunk, Mem *mem);
