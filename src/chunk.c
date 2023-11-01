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
  u32 longest_sym, width, col1, col2;
  u32 i, line;
  char *sym = chunk->symbols.names;

  longest_sym = Min(17, LongestSymbol(&chunk->symbols));
  col1 = 20 + longest_sym;
  col2 = col1 + 6 + longest_sym;
  width = col2 + longest_sym + 2;

  printf("╔");
  for (i = 0; i < width-2; i++) printf("═");
  printf("╗\n");

  printf("║  src  idx  Instruction");
  for (i = 24; i < col1; i++) printf(" ");
  i += printf("Constants");
  while (i < col2) i += printf(" ");
  i += printf("Symbols");
  while (i < width - 1) i += printf(" ");
  printf("║\n");

  printf("╟─────┬────┬");
  for (i = 12; i < width-1; i++) printf("─");
  printf("╢\n");

  line = 0;
  sym = chunk->symbols.names;
  for (i = 0; i < chunk->count; i += OpLength(ChunkRef(chunk, i))) {
    u8 op = ChunkRef(chunk, i);
    u32 j, k;
    u32 source = GetSourcePosition(i, chunk);

    printf("║");
    printf(" %4d│%4d│", source, i);
    k = 12;
    k += printf(" %s", OpName(op));
    for (j = 0; j < OpLength(op) - 1; j++) {
      k += printf(" ");
      k += PrintVal(ChunkConst(chunk, i+j+1), &chunk->symbols);
    }

    while (k < col1) k += printf(" ");
    if (line < chunk->num_constants) {
      Val value = chunk->constants[line];
      k += printf("%3d: ", line);
      if (IsSym(value)) {
        k += printf("%.*s", longest_sym, SymbolName(value, &chunk->symbols));
      } else {
        k += PrintVal(chunk->constants[line], &chunk->symbols);
      }
    }

    while (k < col2) k += printf(" ");
    if (sym < chunk->symbols.names + chunk->symbols.count) {
      u32 length = StrLen(sym);
      k += printf("%.*s", longest_sym, sym);
      sym += length + 1;
    }

    while (k < width - 1) k += printf(" ");
    printf("║\n");
    line++;
  }

  printf("╚");
  for (i = 0; i < width-2; i++) printf("═");
  printf("╝\n");
}
