#pragma once
#include "heap.h"
#include "opts.h"
#include "univ/hashmap.h"

typedef struct {
  char *filename;
  u32 offset;
} FileOffset;

typedef struct {
  u32 capacity;
  u32 count;
  u8 *data;
  Heap constants;
  HashMap source_map;
  u32 sources_capacity;
  u32 sources_count;
  FileOffset *sources;
} Chunk;

#define CurrentVersion  1

void InitChunk(Chunk *chunk);
void DestroyChunk(Chunk *chunk);
void FreeChunk(Chunk *chunk);

u32 ChunkSize(Chunk *chunk);
u8 ChunkRef(Chunk *chunk, u32 i);
Val ChunkConst(Chunk *chunk, u32 i);
Val *ChunkBinary(Chunk *chunk, u32 i);

void PushByte(u8 byte, Chunk *chunk);
void PushConst(Val value, Chunk *chunk);

u32 SerializedSize(Chunk *chunk);
u8 *SerializeChunk(Chunk *chunk);
void DeserializeChunk(u8 *data, Chunk *chunk);

void AddSourceFile(char *filename, u32 offset, Chunk *chunk);
char *SourceFile(u32 pos, Chunk *chunk);
u32 SourcePos(u32 pos, Chunk *chunk);

Chunk *LoadChunk(CassetteOpts *opts);
