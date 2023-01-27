#pragma once
#include "value.h"

enum Reg;

typedef enum {
  OP_RETURN,
  OP_CONST,
} OpCode;

typedef enum {
  ARGS_NONE,
  ARGS_VAL,
} ArgFormat;

typedef struct {
  u8 *code;
  u32 *lines;
  Val *constants;
} Chunk;

Chunk *NewChunk(void);
Chunk *InitChunk(Chunk *chunk);
void FreeChunk(Chunk *chunk);
u32 PutByte(Chunk *chunk, u32 line, u8 byte);
u8 GetByte(Chunk *chunk, u32 i);
u8 PutConst(Chunk *chunk, Val value);
Val GetConst(Chunk *chunk, u32 i);
u32 PutInst(Chunk *chunk, u32 line, OpCode op, ...);
void Disassemble(char *title, Chunk *chunk);
