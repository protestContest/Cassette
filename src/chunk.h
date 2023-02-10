#pragma once
#include "ops.h"
#include "value.h"
#include "mem.h"
#include "string.h"

typedef struct {
  u8 *code;
  Val *constants;
  StringMap strings;
  Symbol *symbols;
} Chunk;

Chunk *NewChunk(void);
Chunk *InitChunk(Chunk *chunk);
void ResetChunk(Chunk *chunk);
void FreeChunk(Chunk *chunk);

u32 ChunkSize(Chunk *chunk);
u32 PutByte(Chunk *chunk, u8 byte);
void SetByte(Chunk *chunk, u32 i, u8 byte);
u8 GetByte(Chunk *chunk, u32 i);
u8 PutConst(Chunk *chunk, Val value);
Val GetConst(Chunk *chunk, u32 i);
Val PutSymbol(Chunk *chunk, char *name, u32 length);
u32 PutInst(Chunk *chunk, OpCode op, ...);
u32 DisassembleInstruction(Chunk *chunk, u32 i);
void Disassemble(char *title, Chunk *chunk);
bool ChunksEqual(Chunk *a, Chunk *b);
void WriteChunk(Chunk *chunk, u32 num_instructions, ...);
void DumpConstants(Chunk *chunk);
