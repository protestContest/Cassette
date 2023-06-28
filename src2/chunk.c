#include "chunk.h"
#include "vm.h"

void InitChunk(Chunk *chunk)
{
  chunk->data = NULL;
  InitMem(&chunk->constants);
}

u8 PushConst(Chunk *chunk, Val value)
{
  for (u32 i = 0; i < VecCount(chunk->constants.values); i++) {
    if (Eq(chunk->constants.values[i], value)) return i;
  }
  u32 n = VecCount(chunk->constants.values);
  VecPush(chunk->constants.values, value);
  return n;
}

void PushByte(Chunk *chunk, u8 byte)
{
  VecPush(chunk->data, byte);
}

void Disassemble(Chunk *chunk)
{
  for (u32 i = 0; i < VecCount(chunk->data);) {
    PrintIntN(i, 3, ' ');
    Print(" ");
    PrintInstruction(chunk, i);
    Print("\n");
    i += OpLength(chunk->data[i]);
  }
}
