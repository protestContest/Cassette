#pragma once
#include "mem.h"

typedef struct {
  u8 *data;
  Mem constants;
} Chunk;

#define ChunkRef(chunk, i)    ((chunk)->data[i])
#define ChunkConst(chunk, i)  ((chunk)->constants.values[ChunkRef(chunk, i)])

void InitChunk(Chunk *chunk);
void DestroyChunk(Chunk *chunk);
u8 PushConst(Chunk *chunk, Val value);
void PushByte(Chunk *chunk, u8 byte);
void AddSymbol(Chunk *chunk, Val symbol, Mem *mem);
void Disassemble(Chunk *chunk);

bool WriteChunk(Chunk *chunk, char *filename);
bool ReadChunk(Chunk *chunk, char *filename);
