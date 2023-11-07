#include "chunk.h"
#include "ops.h"
#include "vm.h"
#include "univ/math.h"
#include "univ/string.h"
#include "univ/system.h"
#include <stdio.h>
#include <stdlib.h>

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

u8 AddConst(Val value, Chunk *chunk)
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

#ifdef DEBUG
void Disassemble(Chunk *chunk)
{
  u32 longest_sym, width, col1, col2;
  u32 i, line;
  char *sym;

  longest_sym = Min(17, LongestSymbol(&chunk->symbols));
  col1 = 20 + longest_sym;
  col2 = col1 + 6 + longest_sym;
  width = col2 + longest_sym + 2;

  printf("╔ Disassembled ");
  for (i = 0; i < width-16; i++) printf("═");
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
  sym = (char*)chunk->symbols.names.items;
  for (i = 0; i < chunk->code.count; i += OpLength(ChunkRef(chunk, i))) {
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
    if ((u8*)sym < chunk->symbols.names.items + chunk->symbols.names.count) {
      u32 length = StrLen(sym);
      k += printf("%.*s", longest_sym, sym);
      sym += length + 1;
    }

    while (k < width - 1) k += printf(" ");
    printf("║\n");
    line++;
  }

  while ((u8*)sym < chunk->symbols.names.items + chunk->symbols.names.count) {
    u32 length = StrLen(sym);
    printf("║");
    i = 1;
    while (i < col2) i += printf(" ");
    i += printf("%.*s", longest_sym, sym);
    sym += length + 1;

    while (i < width - 1) i += printf(" ");
    printf("║\n");
  }

  printf("╚");
  for (i = 0; i < width-2; i++) printf("═");
  printf("╝\n");
}
#endif
