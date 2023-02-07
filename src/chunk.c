#include "chunk.h"
#include "printer.h"
#include "vec.h"
#include "mem.h"

Chunk *NewChunk(void)
{
  Chunk *chunk = malloc(sizeof(Chunk));
  if (!chunk) Fatal("Out of memory");
  InitChunk(chunk);
  return chunk;
}

Chunk *InitChunk(Chunk *chunk)
{
  chunk->code = NULL;
  chunk->constants = NULL;
  chunk->symbols = NULL;

  PutSymbol(chunk, "true", 4);
  PutSymbol(chunk, "false", 5);
  PutSymbol(chunk, "ok", 2);

  return chunk;
}

void ResetChunk(Chunk *chunk)
{
  FreeVec(chunk->code);
  FreeVec(chunk->constants);
  FreeVec(chunk->symbols);
  InitChunk(chunk);
}

void FreeChunk(Chunk *chunk)
{
  ResetChunk(chunk);
  free(chunk);
}

u32 ChunkSize(Chunk *chunk)
{
  return VecCount(chunk->code);
}

u32 PutByte(Chunk *chunk, u8 byte)
{
  VecPush(chunk->code, byte);
  return VecCount(chunk->code) - 1;
}

void SetByte(Chunk *chunk, u32 i, u8 byte)
{
  chunk->code[i] = byte;
}

u8 GetByte(Chunk *chunk, u32 i)
{
  return chunk->code[i];
}

u8 PutConst(Chunk *chunk, Val value)
{
  for (u32 i = 0; i < VecCount(chunk->constants); i++) {
    if (Eq(chunk->constants[i], value)) return i;
  }

  VecPush(chunk->constants, value);
  return VecCount(chunk->constants) - 1;
}

Val GetConst(Chunk *chunk, u32 i)
{
  return chunk->constants[i];
}

Val PutSymbol(Chunk *chunk, char *name, u32 length)
{
  return MakeSymbolFromSlice(&chunk->symbols, name, length);
}

u32 PutInst(Chunk *chunk, OpCode op, ...)
{
  va_list args;
  va_start(args, op);

  PutByte(chunk, op);

  if (op == OP_CONST) {
    PutByte(chunk, PutConst(chunk, va_arg(args, Val)));
  } else if (OpSize(op) > 1) {
    PutByte(chunk, va_arg(args, u32));
  }

  va_end(args);
  return VecCount(chunk->code) - OpSize(op);
}

u32 DisassembleInstruction(Chunk *chunk, u32 i)
{
  u32 written = printf("%4u │ ", i);

  OpCode op = chunk->code[i];
  switch (OpFormat(op)) {
  case ARGS_VAL:
    written += printf("%02X %02X %s ", GetByte(chunk, i), GetByte(chunk, i+1), OpStr(op));
    written += PrintVal(chunk->constants, chunk->symbols, GetConst(chunk, GetByte(chunk, i + 1)));
    break;
  case ARGS_INT:
    written += printf("%02X %02X %s %d", GetByte(chunk, i), GetByte(chunk, i+1), OpStr(op), GetByte(chunk, i + 1));
    break;
  default:
    written += printf("%02X    %s", GetByte(chunk, i), OpStr(op));
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

bool ChunksEqual(Chunk *a, Chunk *b)
{
  if (VecCount(a->code) != VecCount(b->code)) return false;
  if (VecCount(a->constants) != VecCount(b->constants)) return false;

  for (u32 i = 0; i < VecCount(a->code); i++) {
    if (a->code[i] != b->code[i]) return false;
  }

  for (u32 i = 0; i < VecCount(a->constants); i++) {
    if (!Eq(a->constants[i], b->constants[i])) return false;
  }

  return true;
}

void WriteChunk(Chunk *chunk, u32 num_instructions, ...)
{
  va_list args;
  va_start(args, num_instructions);
  for (u32 i = 0; i < num_instructions; i++) {
    u8 op = va_arg(args, u32);
    PutByte(chunk, op);

    if (op == OP_CONST) {
      PutByte(chunk, PutConst(chunk, va_arg(args, Val)));
    } else if (OpSize(op) > 1) {
      PutByte(chunk, va_arg(args, u32));
    }
  }
  va_end(args);
}

void DumpConstants(Chunk *chunk)
{
  for (u32 i = 0; i < VecCount(chunk->constants); i++) {
    printf("%02d  ", i);
    PrintVal(chunk->constants, chunk->symbols, chunk->constants[i]);
    printf("\n");
  }
}
