#pragma once
#include "../value.h"

typedef struct {
  u8 *code;
  Val *constants;
  Val symbols;
} Chunk;

void InitChunk(Chunk *chunk);
u32 ChunkSize(Chunk *chunk);

u32 PutByte(Chunk *chunk, u8 byte);
void SetByte(Chunk *chunk, u32 i, u8 byte);
u8 GetByte(Chunk *chunk, u32 i);
u8 PutConst(Chunk *chunk, Val value);
Val GetConst(Chunk *chunk, u32 i);
void PutSymbol(Chunk *chunk, Val symbol, Val name);

u32 DisassembleInstruction(Chunk *chunk, u32 i);
void Disassemble(Chunk *chunk);
