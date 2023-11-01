#pragma once
#include "mem.h"
#include "symbols.h"

#define MaxConstants  256

typedef struct {
  u32 capacity;
  u32 count;
  u32 *sources;
} SourceMap;

typedef struct {
  u32 capacity;
  u32 count;
  u8 *code;
  u32 num_constants;
  Val constants[MaxConstants];
  SymbolTable symbols;
  SourceMap source_map;
} Chunk;

#define ChunkRef(chunk, i)        ((chunk)->code[i])
#define ChunkConst(chunk, i)      ((chunk)->constants[ChunkRef(chunk, i)])

void InitChunk(Chunk *chunk);
void DestroyChunk(Chunk *chunk);

u32 PushByte(u8 byte, u32 source_pos, Chunk *chunk);
u8 AddConst(Val value, Chunk *chunk);
void PushConst(Val value, u32 source_pos, Chunk *chunk);
void PatchChunk(Chunk *chunk, u32 index, Val value);

u32 GetSourcePosition(u32 byte_pos, Chunk *chunk);
void Disassemble(Chunk *chunk);
