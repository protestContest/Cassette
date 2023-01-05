#pragma once
#include "value.h"

typedef enum {
  OP_ASSIGN,
  OP_MOVE,
  OP_TEST,
  OP_BRANCH,
  OP_GOTO,
  OP_SAVE,
  OP_RESTORE,
  OP_PERFORM,
  OP_HALT,
  OP_CMP,
  OP_REM,
  OP_BREAK
} OpCode;

typedef struct {
  u32 count;
  u32 capacity;
  u8 *code;
  struct {
    u32 count;
    u32 capacity;
    Val *values;
  } constants;
} Chunk;

typedef enum {
  REG_A,
  REG_B,
  REG_T,
  REG_Z,
  REG_ADDR
} Register;

typedef struct {
  bool flag;
  u32 pc;
  u32 sp;
  Val *stack;
  Val registers[16];
  Chunk *chunk;
} VM;

Chunk *NewChunk(void);
void PutChunk(Chunk *chunk, u8 byte);
u8 PutConstant(Chunk *chunk, Val constant);
Val ChunkConstant(Chunk *chunk, u8 index);

void Disassemble(Chunk *chunk);

void Assign(Chunk *chunk, u8 constant, Register dst);
void Move(Chunk *chunk, Register src, Register dst);
void Test(Chunk *chunk, Register reg);
void Branch(Chunk *chunk, Register reg);
void Goto(Chunk *chunk, Register reg);
void Save(Chunk *chunk, Register reg);
void Restore(Chunk *chunk, Register reg);
void Perform(Chunk *chunk);
void Halt(Chunk *chunk);
void Compare(Chunk *chunk, Register a, Register b);
void Rem(Chunk *chunk, Register a, Register b, Register dst);
void Break(Chunk *chunk);

VM *NewVM(void);
void ExecuteChunk(VM *vm, Chunk *chunk);

