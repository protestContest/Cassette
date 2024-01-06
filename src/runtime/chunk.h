#pragma once
#include "mem/mem.h"
#include "mem/symbols.h"
#include "univ/vec.h"

#define MaxConstants  1024

typedef struct {
  ByteVec code;
  u32 num_constants;
  Val constants[MaxConstants];
  SymbolTable symbols;
  IntVec source_map;
  IntVec file_map;
  u32 num_modules;
} Chunk;

#define ChunkRef(chunk, i)        ((chunk)->code.items[i])
#define ChunkConst(chunk, i)      ((chunk)->constants[ChunkRef(chunk, i)])

void InitChunk(Chunk *chunk);
void DestroyChunk(Chunk *chunk);

void BeginChunkFile(Val filename, Chunk *chunk);
void EndChunkFile(Chunk *chunk);
char *ChunkFile(u32 pos, Chunk *chunk);
u32 ChunkFileLength(u32 pos, Chunk *chunk);

u32 PushByte(u8 byte, u32 source_pos, Chunk *chunk);
u32 PushConst(Val value, u32 source_pos, Chunk *chunk);
void PatchConst(Chunk *chunk, u32 index);

u32 GetSourcePosition(u32 byte_pos, Chunk *chunk);
