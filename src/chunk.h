#pragma once
#include "ops.h"
#include "value.h"

typedef struct {
  u8 *code;
  u32 *lines;
  Val *heap;
} Chunk;

Chunk *NewChunk(void);
Chunk *InitChunk(Chunk *chunk);
void ResetChunk(Chunk *chunk);
void FreeChunk(Chunk *chunk);

u32 PutByte(Chunk *chunk, u32 line, u8 byte);
u8 GetByte(Chunk *chunk, u32 i);
u8 PutConst(Chunk *chunk, Val value);
Val GetConst(Chunk *chunk, u32 i);
u32 PutInst(Chunk *chunk, u32 line, OpCode op, ...);
u32 DisassembleInstruction(Chunk *chunk, u32 i);
void Disassemble(char *title, Chunk *chunk);
