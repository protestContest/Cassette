#include "chunk.h"
#include "univ/system.h"

static u8 AddConst(Val value, Chunk *chunk);

void InitChunk(Chunk *chunk)
{
  InitVec((Vec*)&chunk->code, sizeof(u8), 256);
  chunk->num_constants = 0;
  InitSymbolTable(&chunk->symbols);
  InitVec((Vec*)&chunk->source_map, sizeof(u32), 256);
  InitVec((Vec*)&chunk->file_map, sizeof(u32), 8);
}

void DestroyChunk(Chunk *chunk)
{
  DestroyVec((Vec*)&chunk->code);
  DestroySymbolTable(&chunk->symbols);
  DestroyVec((Vec*)&chunk->source_map);
  DestroyVec((Vec*)&chunk->file_map);
  chunk->num_constants = 0;
}

void BeginChunkFile(Val filename, Chunk *chunk)
{
  IntVecPush(&chunk->file_map, filename);
  IntVecPush(&chunk->file_map, 0);
}

void EndChunkFile(Chunk *chunk)
{
  u32 prev_size = 0, i;
  /* count up the size of modules previous */
  for (i = 0; i < chunk->file_map.count; i += 2) {
    prev_size += chunk->file_map.items[i+1];
  }

  /* patch the last module size */
  chunk->file_map.items[chunk->file_map.count-1] = chunk->code.count - prev_size;
}

char *ChunkFile(u32 pos, Chunk *chunk)
{
  u32 size = 0;
  u32 i;

  for (i = 0; i < chunk->file_map.count; i += 2) {
    if (size + chunk->file_map.items[i+1] > pos) {
      return SymbolName(chunk->file_map.items[i], &chunk->symbols);
    }
    size += chunk->file_map.items[i+1];
  }

  return 0;
}

#define LastSourcePos(chunk)    ((chunk)->source_map.items[(chunk)->source_map.count-2])
u32 PushByte(u8 byte, u32 source_pos, Chunk *chunk)
{
  u32 pos = chunk->code.count;

  ByteVecPush(&chunk->code, byte);

  /* the source map is an RLE encoded list of source positions */
  if (chunk->source_map.count == 0 || LastSourcePos(chunk) != source_pos) {
    IntVecPush(&chunk->source_map, source_pos);
    IntVecPush(&chunk->source_map, 1);
  } else {
    /* increment the position counter */
    chunk->source_map.items[chunk->source_map.count-1]++;
  }

  return pos;
}

static u8 AddConst(Val value, Chunk *chunk)
{
  u32 i;

  /* check if we already have this constant */
  for (i = 0; i < chunk->num_constants; i++) {
    if (value == chunk->constants[i]) {
      return i;
    }
  }

  /* TODO: support more constants */
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
  chunk->code.items[index] = AddConst(value, chunk);
}

void PatchJump(Chunk *chunk, u32 index)
{
  PatchChunk(chunk, index+1, IntVal(chunk->code.count - index));
}

u32 GetSourcePosition(u32 byte_pos, Chunk *chunk)
{
  IntVec map = chunk->source_map;
  u32 i;

  for (i = 0; i < map.count; i += 2) {
    if (map.items[i+1] > byte_pos) return map.items[i];
    byte_pos -= map.items[i+1];
  }

  return -1;
}
