#include "chunk.h"
#include "../runtime/print.h"
#include "../runtime/ops.h"
#include "../runtime/mem.h"
#include <univ/io.h>
#include <univ/str.h>
#include <univ/vec.h>

void InitChunk(Chunk *chunk)
{
  chunk->code = NULL;
  InitMem(&chunk->constants);
  chunk->symbols = MakeMap(&chunk->constants, 32);
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

void PutSymbol(Chunk *chunk, Val symbol, Val name)
{
  MapPut(chunk->constants, chunk->symbols, symbol, name);
}

u32 DisassembleInstruction(Chunk *chunk, u32 i)
{
  PrintInt(i, 4);
  Print(" │ ");
  u32 written = 7;

  OpCode op = chunk->code[i];
  switch (OpFormat(op)) {
  case ARGS_VAL: {
    Print(OpStr(op));
    Print(" ");
    written += StrLen(OpStr(op)) + 1;
    written += PrintVal(chunk->constants, chunk->symbols, GetConst(chunk, GetByte(chunk, i + 1)));
    break;
  }
  case ARGS_INT:
    Print(OpStr(op));
    Print(" ");
    written += StrLen(OpStr(op)) + 1;
    PrintInt(GetByte(chunk, i + 1), 0);
    written += NumDigits(GetByte(chunk, i + 1));
    break;
  default:
    Print(OpStr(op));
    written += StrLen(OpStr(op));
    break;
  }

  return written;
}

void Disassemble(Chunk *chunk)
{
  Print("───╴Chunk╶───\n");

  for (u32 i = 0; i < VecCount(chunk->code); i += OpSize(chunk->code[i])) {
    DisassembleInstruction(chunk, i);
    Print("\n");
  }
  Print("\n");
}
