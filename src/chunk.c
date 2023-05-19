#include "chunk.h"
#include "vm.h"
#include "ops.h"
#include "print.h"

void InitChunk(Chunk *chunk)
{
  chunk->code = NULL;
  chunk->constants = NULL;
  InitStringTable(&chunk->symbols);
}

static void PushOp(OpCode op, Chunk *chunk)
{
  VecPush(chunk->code, op);
}

static void PushConst(Val value, Chunk *chunk)
{
  for (u32 i = 0; i < VecCount(chunk->constants); i++) {
    if (Eq(value, chunk->constants[i])) {
      VecPush(chunk->code, i);
      return;
    }
  }

  VecPush(chunk->code, VecCount(chunk->constants));
  VecPush(chunk->constants, value);
}

static void PushReg(Val reg, Chunk *chunk)
{
  VecPush(chunk->code, RawInt(reg));
}

static void PushData(Val binary, Chunk *chunk, Mem *mem)
{
  // u32 position = VecCount(chunk->symbols.names);
  // PutSymbol(&chunk->symbols, (char*)BinaryData(mem, binary), BinaryLength(mem, binary));
  // PushConst(IntVal(position), chunk);
}

static Val AssembleInstruction(Val stmts, Chunk *chunk, Mem *mem)
{
  Val op_sym = Head(mem, stmts);

  for (u32 op = 0; op < NUM_OPCODES; op++) {
    if (Eq(op_sym, OpSymbol(op))) {
#if DEBUG_ASSEMBLE
      u32 inst_start = VecCount(chunk->data);
#endif
      PushOp(op, chunk);

#if DEBUG_ASSEMBLE
      for (u32 i = 0; i < OpLength(op); i++) {
        PrintVal(mem, ListAt(mem, stmts, i));
        Print(" ");
      }
#endif

      Val result;
      switch (OpArgs(op)) {
      case ArgsNone:
        result = Tail(mem, stmts);
        break;
      case ArgsConst:
        PushConst(ListAt(mem, stmts, 1), chunk);
        result = ListFrom(mem, stmts, 2);
        break;
      case ArgsReg:
        PushReg(Tail(mem, ListAt(mem, stmts, 1)), chunk);
        result = ListFrom(mem, stmts, 2);
        break;
      case ArgsConstReg:
        PushConst(ListAt(mem, stmts, 1), chunk);
        PushReg(Tail(mem, ListAt(mem, stmts, 2)), chunk);
        result = ListFrom(mem, stmts, 3);
        break;
      case ArgsConstConstReg:
        PushConst(Second(mem, stmts), chunk);
        PushConst(Third(mem, stmts), chunk);
        PushReg(Tail(mem, Fourth(mem, stmts)), chunk);
        result = ListFrom(mem, stmts, 4);
        break;
      case ArgsRegReg:
        PushReg(Tail(mem, ListAt(mem, stmts, 1)), chunk);
        PushReg(Tail(mem, ListAt(mem, stmts, 2)), chunk);
        result = ListFrom(mem, stmts, 3);
        break;
      }

#if DEBUG_ASSEMBLE
      Print("-> ");
      PrintInstruction(chunk, inst_start, mem);
      Print("\n");
#endif

      return result;
    }
  }

  Print("Unknown operation: ");
  PrintVal(mem, op_sym);
  Print("\n");

  Assert(false);
}

Chunk Assemble(Val stmts, Mem *mem)
{
  Chunk chunk;
  InitChunk(&chunk);

  while (!IsNil(stmts)) {
    stmts = AssembleInstruction(stmts, &chunk, mem);
  }

  return chunk;
}

u8 ChunkRef(Chunk *chunk, u32 index)
{
  Assert(index < VecCount(chunk->code));
  return chunk->code[index];
}

Val ChunkConst(Chunk *chunk, u32 index)
{
  Assert(index < VecCount(chunk->code));
  index = chunk->code[index];
  Assert(index < VecCount(chunk->constants));
  return chunk->constants[index];
}

u32 PrintInstruction(Chunk *chunk, u32 i, Mem *mem)
{
  u32 printed = 0;

  OpCode op = chunk->code[i];
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
  while (i < VecCount(chunk->code)) {
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
