#include "chunk.h"
#include "univ/system.h"
#include "ops.h"
#include "debug.h"
#include "univ/math.h"

static u32 AddConst(Val value, Chunk *chunk);

void InitChunk(Chunk *chunk)
{
  InitVec((Vec*)&chunk->code, sizeof(u8), 256);
  chunk->num_constants = 0;
  InitSymbolTable(&chunk->symbols);
  DefinePrimitiveSymbols(&chunk->symbols);
  InitVec((Vec*)&chunk->source_map, sizeof(u8), 256);
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
  ByteVecPush(&chunk->source_map, 0);
  ByteVecPush(&chunk->source_map, 0);
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

char *ChunkFileAt(u32 pos, Chunk *chunk)
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

u32 ChunkFileByteSize(u32 pos, Chunk *chunk)
{
  u32 size = 0;
  u32 i;

  for (i = 0; i < chunk->file_map.count; i += 2) {
    if (size + chunk->file_map.items[i+1] > pos) {
      return chunk->file_map.items[i+1];
    }
    size += chunk->file_map.items[i+1];
  }

  return 0;
}

#define SourceMapPeek(chunk, i)    ((chunk)->source_map.items[(chunk)->source_map.count-(i)])
u32 PushByte(u8 byte, Chunk *chunk)
{
  u32 pos = chunk->code.count;

  ByteVecPush(&chunk->code, byte);

  chunk->source_map.items[chunk->source_map.count-1]++;
  if (chunk->source_map.items[chunk->source_map.count-1] == 127) {
    PushSourcePos(0, chunk);
  }

  return pos;
}

static i32 FindConst(Val value, Chunk *chunk)
{
  i32 i;
  for (i = 0; i < (i32)chunk->num_constants; i++) {
    if (value == chunk->constants[i]) {
      return i;
    }
  }
  return -1;
}

static u32 AddConst(Val value, Chunk *chunk)
{
  chunk->constants[chunk->num_constants++] = value;
  return chunk->num_constants - 1;
}

u32 PushConst(Val value, Chunk *chunk)
{
  u32 pos = chunk->code.count;
  if (IsInt(value) && RawInt(value) >= 0 && RawInt(value) <= 255) {
    u8 byte = RawInt(value);
    PushByte(OpInt, chunk);
    PushByte(byte, chunk);
  } else if (value == Nil) {
    PushByte(OpNil, chunk);
  } else {
    i32 index = FindConst(value, chunk);
    if (index < 0) {
      index = AddConst(value, chunk);
    }

    if (index < 256) {
      PushByte(OpConst, chunk);
      PushByte((u8)index, chunk);
    } else {
      PushByte(OpConst2, chunk);
      PushByte((u8)((index >> 8) & 0xFF), chunk);
      PushByte((u8)(index & 0xFF), chunk);
    }
  }
  return pos;
}

void PatchConst(Chunk *chunk, u32 index)
{
  u32 dist = chunk->code.count - index - 3;
  if (dist < 256) {
    chunk->code.items[index] = OpInt;
    chunk->code.items[index+1] = (u8)dist;
  } else if (chunk->num_constants < 256) {
    chunk->code.items[index] = OpConst;
    chunk->code.items[index+1] = AddConst(IntVal(dist), chunk);
  } else {
    u32 const_idx = AddConst(IntVal(dist), chunk);
    chunk->code.items[index] = OpConst2;
    chunk->code.items[index+1] = (u8)((const_idx >> 8) & 0xFF);
    chunk->code.items[index+2] = (u8)(const_idx & 0xFF);
  }
}

void PushSourcePos(i32 pos, Chunk *chunk)
{
  if (chunk->source_map.count > 0 && pos == 0) return;

  if (SourceMapPeek(chunk, 1) == 0) {
    pos += (i8)SourceMapPeek(chunk, 2);
    chunk->source_map.count -= 2;
  }

  while (pos > 127) {
    ByteVecPush(&chunk->source_map, 127);
    ByteVecPush(&chunk->source_map, 0);
    pos -= 127;
  }
  while (pos < -128) {
    ByteVecPush(&chunk->source_map, -128);
    ByteVecPush(&chunk->source_map, 0);
    pos += 128;
  }
  ByteVecPush(&chunk->source_map, (u8)pos);
  ByteVecPush(&chunk->source_map, 0);
}

u32 GetSourcePosition(u32 byte_pos, Chunk *chunk)
{
  ByteVec map = chunk->source_map;
  u32 i, pos = 0, bytes = 0;

  for (i = 0; i < map.count; i += 2) {
    pos += map.items[i];
    if (bytes + map.items[i+1] > byte_pos) {
      return pos;
    }

    bytes += map.items[i+1];
  }

  return -1;
}
