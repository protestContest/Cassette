#pragma once
#include "mem.h"

typedef struct {
  u8 *data;
  Mem constants;
} Chunk;

#define ChunkRef(chunk, i)    ((chunk)->data[i])
#define ChunkConst(chunk, n)  ((chunk)->constants.values[n])

void InitChunk(Chunk *chunk);
u8 PushConst(Chunk *chunk, Val value);
void PushByte(Chunk *chunk, u8 byte);
void Disassemble(Chunk *chunk);
