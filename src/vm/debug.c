#include "debug.h"
#include "ops.h"
#include "debug.h"

static u32 PrintInstruction(Chunk *chunk, u32 index)
{
  OpCode op = chunk->data[index];
  u32 length = Print(OpName(op));
  for (u32 i = 1; i < OpLength(op); i++) {
    Val arg = ChunkConst(chunk, index + i);
    length += Print(" ");
    length += Inspect(arg, &chunk->constants);
  }

  return length;
}

void TraceInstruction(VM *vm, Chunk *chunk)
{
  PrintIntN(vm->pc, 4);
  Print("│ ");

  u32 written = 0;
  if (vm->pc < ChunkSize(chunk)) {
    written = PrintInstruction(chunk, vm->pc);
  }

  if (written < 20) {
    for (u32 i = 0; i < 20 - written; i++) Print(" ");
  }

  Print(" │ ");
  PrintIntN(vm->cont, 4);
  Print(" ║ ");

  for (u32 i = 0; i < VecCount(vm->stack) && i < 8; i++) {
    DebugVal(vm->stack[i], vm->mem);
    Print(" │ ");
  }

  Print("\n");
}
