#pragma once
#include "heap.h"

typedef struct {
  u8 *data;
  Heap constants;
  u32 num_modules;
} Chunk;

void InitChunk(Chunk *chunk);
void DestroyChunk(Chunk *chunk);

u32 ChunkSize(Chunk *chunk);
u8 ChunkRef(Chunk *chunk, u32 i);
Val ChunkConst(Chunk *chunk, u32 i);
Val *ChunkBinary(Chunk *chunk, u32 i);

void PushByte(u8 byte, Chunk *chunk);
void PushConst(Val value, Chunk *chunk);

u8 *SerializeChunk(Chunk *chunk);
void DeserializeChunk(u8 *data, Chunk *chunk);
