#include "chunk.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>

static void ResizeChunk(Chunk *chunk, u32 capacity)
{
  chunk->code = realloc(chunk->code, capacity);
  chunk->capacity = capacity;
}

void InitChunk(Chunk *chunk)
{
  chunk->capacity = 0;
  chunk->count = 0;
  chunk->code = 0;
  chunk->num_constants = 0;
  ResizeChunk(chunk, 256);
  InitSymbolTable(&chunk->symbols);
}

void ResetChunk(Chunk *chunk)
{
  chunk->count = 0;
  chunk->num_constants = 0;
}

void DestroyChunk(Chunk *chunk)
{
  if (chunk->code != 0) free(chunk->code);
  ResetChunk(chunk);
}

u32 PushByte(u8 byte, Chunk *chunk)
{
  u32 pos = chunk->count;
  if (chunk->count + 1 == chunk->capacity) {
    ResizeChunk(chunk, chunk->capacity * 2);
  }

  ChunkRef(chunk, chunk->count++) = byte;
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

void PushConst(Val value, Chunk *chunk)
{
  PushByte(AddConst(value, chunk), chunk);
}

void PatchChunk(Chunk *chunk, u32 index, Val value)
{
  chunk->code[index] = AddConst(value, chunk);
}

void DumpConstants(Chunk *chunk)
{
  u32 i;
  for (i = 0; i < chunk->num_constants; i++) {
    PrintVal(chunk->constants[i], 0);
    if (i < chunk->num_constants-1) printf(", ");
  }
  printf("\n");
}

void DumpChunk(Chunk *chunk)
{
  u32 i, row, ncols = 16, nrows = chunk->count / ncols;

  for (row = 0; row < nrows; row++) {
    u32 end = Min(chunk->count, (row+1)*ncols);
    printf("%04X:", row*ncols);
    for (i = row*ncols; i < end; i++) {
      printf(" %02X", ChunkRef(chunk, i));
    }
    printf("\n");
  }
  printf("%04X:", row*ncols);
  for (i = nrows*ncols; i < chunk->count; i++) {
    printf(" %02X", ChunkRef(chunk, i));
  }
  printf("\n");
}

void Disassemble(Chunk *chunk)
{
  u32 i = 0;
  for (i = 0; i < chunk->count; i += OpLength(ChunkRef(chunk, i))) {
    u8 op = ChunkRef(chunk, i);
    u32 j;
    printf("%3d│ %s", i, OpName(op));
    for (j = 0; j < OpLength(op) - 1; j++) {
      printf(" ");
      PrintVal(ChunkConst(chunk, i+j+1), 0);
    }
    printf("\n");
  }
}
