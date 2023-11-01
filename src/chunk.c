#include "chunk.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>

static void ResizeChunk(Chunk *chunk, u32 capacity)
{
  chunk->code = realloc(chunk->code, capacity);
  chunk->capacity = capacity;
}

static void ResizeSourceMap(Chunk *chunk, u32 capacity)
{
  chunk->source_map.sources = realloc(chunk->source_map.sources, sizeof(u32)*capacity);
  chunk->source_map.capacity = capacity;
}

void InitChunk(Chunk *chunk)
{
  chunk->capacity = 0;
  chunk->count = 0;
  chunk->code = 0;
  chunk->num_constants = 0;
  ResizeChunk(chunk, 256);
  InitSymbolTable(&chunk->symbols);
  chunk->source_map.capacity = 256;
  chunk->source_map.count = 0;
  chunk->source_map.sources = malloc(sizeof(u32)*chunk->source_map.capacity);
}

void DestroyChunk(Chunk *chunk)
{
  chunk->capacity = 0;
  chunk->count = 0;
  if (chunk->code != 0) free(chunk->code);
  chunk->code = 0;
  DestroySymbolTable(&chunk->symbols);
  chunk->source_map.capacity = 0;
  chunk->source_map.count = 0;
  if (chunk->source_map.sources != 0) free(chunk->source_map.sources);
  chunk->source_map.sources = 0;
}

u32 PushByte(u8 byte, u32 source_pos, Chunk *chunk)
{
  u32 pos = chunk->count;
  if (chunk->count + 1 == chunk->capacity) {
    ResizeChunk(chunk, chunk->capacity * 2);
  }

  ChunkRef(chunk, chunk->count++) = byte;

  if (chunk->source_map.count > 2 && chunk->source_map.sources[chunk->source_map.count-2] == source_pos) {
    chunk->source_map.sources[chunk->source_map.count-1]++;
  } else {
    if (chunk->source_map.count+2 >= chunk->source_map.capacity) ResizeSourceMap(chunk, 2*chunk->source_map.capacity);
    chunk->source_map.sources[chunk->source_map.count++] = source_pos;
    chunk->source_map.sources[chunk->source_map.count++] = 1;
  }

  return pos;
}

u8 AddConst(Val value, Chunk *chunk)
{
  u32 i;
  for (i = 0; i < chunk->num_constants; i++) {
    if (value == chunk->constants[i]) {
      return i;
    }
  }

  Assert(chunk->num_constants < MaxConstants);
  chunk->constants[chunk->num_constants++] = value;
  return chunk->num_constants - 1;
}

void PushConst(Val value, u32 source_pos, Chunk *chunk)
{
  PushByte(AddConst(value, chunk), source_pos, chunk);
}

void PatchChunk(Chunk *chunk, u32 index, Val value)
{
  chunk->code[index] = AddConst(value, chunk);
}

u32 GetSourcePosition(u32 byte_pos, Chunk *chunk)
{
  SourceMap map = chunk->source_map;
  u32 i;

  for (i = 0; i < map.count; i += 2) {
    if (map.sources[i+1] > byte_pos) return map.sources[i];
    byte_pos -= map.sources[i+1];
  }

  return -1;
}

void Disassemble(Chunk *chunk)
{
  u32 i = 0;
  for (i = 0; i < chunk->count; i += OpLength(ChunkRef(chunk, i))) {
    u8 op = ChunkRef(chunk, i);
    u32 j;
    u32 source = GetSourcePosition(i, chunk);

    printf("%3d│%3d│ %s", source, i, OpName(op));
    for (j = 0; j < OpLength(op) - 1; j++) {
      printf(" ");
      PrintVal(ChunkConst(chunk, i+j+1), &chunk->symbols);
    }
    printf("\n");
  }
}
