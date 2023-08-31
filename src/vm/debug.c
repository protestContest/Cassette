#include "debug.h"
#include "ops.h"
#include "function.h"

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

void PrintEnv(Val env, Heap *mem)
{
  while (!IsNil(env)) {
    Val frame = Head(env, mem);
    for (u32 i = 0; i < TupleLength(frame, mem); i++) {
      Val value = TupleGet(frame, i, mem);
      if (IsFunc(value, mem)) {
        Print("λ");
        Inspect(ListAt(value, 1, mem), mem);
      } else {
        Inspect(value, mem);
      }
      if (i < TupleLength(frame, mem) - 1) {
        Print(", ");
      }
    }
    Print("\n");
    env = Tail(env, mem);
  }
}
