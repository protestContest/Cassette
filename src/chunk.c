#include "chunk.h"
#include "printer.h"
#include "vec.h"

Chunk *NewChunk(void)
{
  Chunk *chunk = malloc(sizeof(Chunk));
  if (!chunk) Error("Out of memory");
  return chunk;
}

Chunk *InitChunk(Chunk *chunk)
{
  chunk->code = NULL;
  chunk->lines = NULL;
  chunk->constants = NULL;
  return chunk;
}

void FreeChunk(Chunk *chunk)
{
  FreeVec(chunk->code);
  FreeVec(chunk->lines);
  FreeVec(chunk->constants);
}

u32 PutByte(Chunk *chunk, u32 line, u8 byte)
{
  VecPush(chunk->code, byte);
  VecPush(chunk->lines, line);
  return VecCount(chunk->code) - 1;
}

u8 GetByte(Chunk *chunk, u32 i)
{
  return chunk->code[i];
}


u8 PutConst(Chunk *chunk, Val value)
{
  VecPush(chunk->constants, value);
  return VecCount(chunk->constants) - 1;
}

Val GetConst(Chunk *chunk, u32 i)
{
  return chunk->constants[i];
}

u32 PutInst(Chunk *chunk, u32 line, OpCode op, ...)
{
  va_list args;
  va_start(args, op);

  PutByte(chunk, line, op);

  switch (op) {
  case OP_CONST:
    PutByte(chunk, line, PutConst(chunk, va_arg(args, Val)));
    break;

  default:
    break;
  }

  va_end(args);
  return VecCount(chunk->code) - OpSize(op);
}

u32 DisassembleInstruction(Chunk *chunk, u32 i)
{
  u32 written = printf("%4u  ", i);
  if (i > 0 && chunk->lines[i] == chunk->lines[i - 1]) {
    written += printf("   │ ");
  } else {
    written += printf("%3u│ ", chunk->lines[i]);
  }

  OpCode op = chunk->code[i];
  switch (OpFormat(op)) {
  case ARGS_VAL:
    written += printf("%s %s", OpStr(op), ValStr(GetConst(chunk, GetByte(chunk, i + 1))));
    break;
  default:
    written += printf("%s", OpStr(op));
    break;

  }

  return written;
}

void Disassemble(char *title, Chunk *chunk)
{
  printf("───╴%s╶───\n", title);

  for (u32 i = 0; i < VecCount(chunk->code); i += OpSize(chunk->code[i])) {
    DisassembleInstruction(chunk, i);
    printf("\n");
  }
  printf("\n");
}
