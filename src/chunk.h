#pragma once
#include "mem.h"

#define MaxConstants  256
typedef struct {
  u32 capacity;
  u32 count;
  u8 *code;
  u32 num_constants;
  Val constants[MaxConstants];
  u8 regs; /* bitfield: [needs ret, needs env, modifies ret, modifies env] */
} Chunk;

#define ChunkRef(chunk, i)        ((chunk)->code[i])
#define ChunkConst(chunk, i)      ((chunk)->constants[ChunkRef(chunk, i)])
#define Needs(reg)                ((reg) << 2)
#define Modifies(reg)             (reg)

void InitChunk(Chunk *chunk);
void ResetChunk(Chunk *chunk);
void DestroyChunk(Chunk *chunk);

void PushByte(u8 byte, Chunk *chunk);
u8 AddConst(Val value, Chunk *chunk);
void PushConst(Val value, Chunk *chunk);
void PatchChunk(Chunk *chunk, u32 index, Val value);

void AppendChunk(Chunk *chunk1, Chunk *chunk2);

void DumpConstants(Chunk *chunk);
void DumpChunk(Chunk *chunk);
void Disassemble(Chunk *chunk);
