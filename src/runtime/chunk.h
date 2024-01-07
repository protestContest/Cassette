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
  ByteVec source_map;
  IntVec file_map;
} Chunk;

#define ChunkRef(chunk, i)        ((chunk)->code.items[i])
#define ChunkConst(chunk, i)      ((chunk)->constants[ChunkRef(chunk, i)])

void InitChunk(Chunk *chunk);
void DestroyChunk(Chunk *chunk);

void BeginChunkFile(Val filename, Chunk *chunk);
void EndChunkFile(Chunk *chunk);
char *ChunkFileAt(u32 pos, Chunk *chunk);
u32 ChunkFileByteSize(u32 pos, Chunk *chunk);

u32 PushByte(u8 byte, Chunk *chunk);
u32 PushConst(Val value, Chunk *chunk);
void PatchConst(Chunk *chunk, u32 index);

void PushSourcePos(i32 pos, Chunk *chunk);
u32 GetSourcePosition(u32 byte_pos, Chunk *chunk);
