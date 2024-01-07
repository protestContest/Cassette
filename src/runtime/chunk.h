#pragma once
#include "mem/mem.h"
#include "mem/symbols.h"
#include "univ/vec.h"
#include "result.h"

typedef struct {
  ByteVec code;
  IntVec constants;
  SymbolTable symbols;
  ByteVec source_map;
  IntVec file_map;
} Chunk;

#define ChunkRef(chunk, i)        ((chunk)->code.items[i])
#define ChunkConst(chunk, i)      ((chunk)->constants.items[ChunkRef(chunk, i)])

void InitChunk(Chunk *chunk);
void DestroyChunk(Chunk *chunk);

void PushFilePos(Val filename, Chunk *chunk);
char *ChunkFileAt(u32 pos, Chunk *chunk);
u32 ChunkFileByteSize(u32 pos, Chunk *chunk);

u32 PushByte(u8 byte, Chunk *chunk);
u32 PushConst(Val value, Chunk *chunk);
void PatchConst(Chunk *chunk, u32 index);

void PushSourcePos(i32 pos, Chunk *chunk);
u32 SourcePosAt(u32 byte_pos, Chunk *chunk);
