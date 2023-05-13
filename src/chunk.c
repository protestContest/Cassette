#include "chunk.h"
#include "vm.h"
#include "ops.h"

static void PushOp(OpCode op, Chunk *chunk)
{
  VecPush(chunk->data, op);
}

static void PushConst(Val value, Chunk *chunk)
{
  for (u32 i = 0; i < VecCount(chunk->constants); i++) {
    if (Eq(value, chunk->constants[i])) {
      VecPush(chunk->data, i);
      return;
    }
  }

  VecPush(chunk->data, VecCount(chunk->constants));
  VecPush(chunk->constants, value);
}

static void PushReg(Val reg, Chunk *chunk)
{
  VecPush(chunk->data, RawInt(reg));
}

static Val AssembleInstruction(Val stmts, Chunk *chunk, Mem *mem)
{
  Val op = Head(mem, stmts);

  for (u32 i = 0; i < NUM_OPCODES; i++) {
    if (Eq(op, OpSymbol(i))) {
#if DEBUG_ASSEMBLE
      u32 inst_start = VecCount(chunk->data);
#endif
      PushOp(i, chunk);

#if DEBUG_ASSEMBLE
      PrintVal(mem, op);
      Print(" ");
#endif

      Val result;
      switch (OpArgs(i)) {
      case ArgsNone:
        result = Tail(mem, stmts);
        break;
      case ArgsConst:
#if DEBUG_ASSEMBLE
        PrintVal(mem, ListAt(mem, stmts, 1));
#endif
        PushConst(ListAt(mem, stmts, 1), chunk);
        result = ListFrom(mem, stmts, 2);
        break;
      case ArgsReg:
#if DEBUG_ASSEMBLE
        PrintVal(mem, ListAt(mem, stmts, 1));
#endif
        PushReg(Tail(mem, ListAt(mem, stmts, 1)), chunk);
        result = ListFrom(mem, stmts, 2);
        break;
      case ArgsConstReg:
#if DEBUG_ASSEMBLE
        PrintVal(mem, Second(mem, stmts));
        Print(" ");
        PrintVal(mem, Third(mem, stmts));
#endif
        PushConst(ListAt(mem, stmts, 1), chunk);
        PushReg(Tail(mem, ListAt(mem, stmts, 2)), chunk);
        result = ListFrom(mem, stmts, 3);
        break;
      case ArgsConstConstReg:
#if DEBUG_ASSEMBLE
        PrintVal(mem, Second(mem, stmts));
        Print(" ");
        PrintVal(mem, Third(mem, stmts));
        Print(" ");
        PrintVal(mem, Fourth(mem, stmts));
#endif
        PushConst(Second(mem, stmts), chunk);
        PushConst(Third(mem, stmts), chunk);
        PushReg(Tail(mem, Fourth(mem, stmts)), chunk);
        result = ListFrom(mem, stmts, 4);
        break;
      case ArgsRegReg:
#if DEBUG_ASSEMBLE
        PrintVal(mem, ListAt(mem, stmts, 1));
        Print(" ");
        PrintVal(mem, ListAt(mem, stmts, 2));
#endif
        PushReg(Tail(mem, ListAt(mem, stmts, 1)), chunk);
        PushReg(Tail(mem, ListAt(mem, stmts, 2)), chunk);
        result = ListFrom(mem, stmts, 3);
        break;
      }

#if DEBUG_ASSEMBLE
      Print(" -> ");
      PrintInstruction(chunk, inst_start, mem);
      Print("\n");
#endif

      return result;
    }
  }

  Print("Unknown operation: ");
  PrintVal(mem, op);
  Print("\n");

  Assert(false);
}

Chunk Assemble(Val stmts, Mem *mem)
{
  Chunk chunk = {NULL, NULL};

  while (!IsNil(stmts)) {
    stmts = AssembleInstruction(stmts, &chunk, mem);
  }

  return chunk;
}

u8 ChunkRef(Chunk *chunk, u32 index)
{
  Assert(index < VecCount(chunk->data));
  return chunk->data[index];
}

Val ChunkConst(Chunk *chunk, u32 index)
{
  Assert(index < VecCount(chunk->data));
  index = chunk->data[index];
  Assert(index < VecCount(chunk->constants));
  return chunk->constants[index];
}

u32 PrintInstruction(Chunk *chunk, u32 i, Mem *mem)
{
  u32 printed = 0;

  OpCode op = chunk->data[i];
  printed += PrintOpCode(op);

  switch (OpArgs(op)) {
  case ArgsNone:
    return printed;
  case ArgsConst:
    printed += Print(" ");
    printed += PrintVal(mem, ChunkConst(chunk, i+1));
    return printed;
  case ArgsReg:
    printed += Print(" ");
    printed += PrintReg(ChunkRef(chunk, i+1));
    return printed;
  case ArgsConstReg:
    printed += Print(" ");
    printed += PrintVal(mem, ChunkConst(chunk, i+1));
    printed += Print(" ");
    printed += PrintReg(ChunkRef(chunk, i+2));
    return printed;
  case ArgsConstConstReg:
    printed += Print(" ");
    printed += PrintVal(mem, ChunkConst(chunk, i+1));
    printed += Print(" ");
    printed += PrintVal(mem, ChunkConst(chunk, i+2));
    printed += Print(" ");
    printed += PrintReg(ChunkRef(chunk, i+3));
    return printed;
  case ArgsRegReg:
    printed += Print(" ");
    printed += PrintReg(ChunkRef(chunk, i+1));
    printed += Print(" ");
    printed += PrintReg(ChunkRef(chunk, i+2));
    return printed;
  }
}

void Disassemble(Chunk *chunk, Mem *mem)
{
  u32 i = 0;
  while (i < VecCount(chunk->data)) {
    PrintIntN(i, 4, ' ');
    Print("│ ");
    PrintInstruction(chunk, i, mem);
    Print("\n");
    i += OpLength(ChunkRef(chunk, i));
  }
  PrintIntN(i, 4, ' ');
  Print("│ <end>\n");
}

void PrintChunkConstants(Chunk *chunk, Mem *mem)
{
  for (u32 i = 0; i < VecCount(chunk->constants); i++) {
    PrintIntN(i, 4, ' ');
    Print("│ ");
    PrintVal(mem, chunk->constants[i]);
    Print("\n");
  }
}
