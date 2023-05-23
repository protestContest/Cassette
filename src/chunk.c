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

static void PushConst(Val value, Chunk *chunk, Mem *mem)
{
  if (IsSym(value)) {
    value = PutString(&chunk->symbols, SymbolName(mem, value), SymbolLength(mem, value));
  }
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
        PushConst(ListAt(mem, stmts, 1), chunk, mem);
        result = ListFrom(mem, stmts, 2);
        break;
      case ArgsReg:
        PushReg(Tail(mem, ListAt(mem, stmts, 1)), chunk);
        result = ListFrom(mem, stmts, 2);
        break;
      case ArgsConstReg:
        PushConst(ListAt(mem, stmts, 1), chunk, mem);
        PushReg(Tail(mem, ListAt(mem, stmts, 2)), chunk);
        result = ListFrom(mem, stmts, 3);
        break;
      case ArgsConstConstReg:
        PushConst(Second(mem, stmts), chunk, mem);
        PushConst(Third(mem, stmts), chunk, mem);
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

void PrintChunk(Chunk *chunk)
{
  HexDump("Code", chunk->code, VecCount(chunk->code));
  HexDump("Constants", (u8*)chunk->constants, VecCount(chunk->constants)*sizeof(Val));
  Print("Symbol map\n");
  for (u32 i = 0; i < VecCount(chunk->symbols.positions); i++) {
    PrintHex(GetMapKey(&chunk->symbols.symbols, i));
    Print(": ");
    PrintInt(GetMapValue(&chunk->symbols.symbols, i));
    Print("\n");
  }
  for (u32 i = 0; i < VecCount(chunk->symbols.positions); i++) {
    PrintInt(chunk->symbols.positions[i]);
    Print(" ");
  }
  Print("\n");
  HexDump("Symbol names", chunk->symbols.data, VecCount(chunk->symbols.data));
}

bool WriteChunk(Chunk *chunk, char *path)
{
  int file = Open(path);
  if (file < 0) return false;

  u32 sig = 0xCA55E77E;
  Write(file, &sig, 4);
  u32 version = 1;
  Write(file, &version, sizeof(u32));

  u32 code_size = VecCount(chunk->code);
  Write(file, &code_size, sizeof(u32));
  Write(file, chunk->code, VecCount(chunk->code));
  u32 num_consts = VecCount(chunk->constants);
  Write(file, &num_consts, sizeof(u32));
  Write(file, chunk->constants, num_consts*sizeof(Val));

  u32 num_syms = VecCount(chunk->symbols.positions);
  Write(file, &num_syms, sizeof(u32));
  for (u32 i = 0; i < num_syms; i++) {
    u32 key = GetMapKey(&chunk->symbols.symbols, i);
    u32 val = GetMapValue(&chunk->symbols.symbols, i);
    Write(file, &key, sizeof(u32));
    Write(file, &val, sizeof(u32));
  }

  Write(file, chunk->symbols.positions, sizeof(u32)*num_syms);

  u32 data_size = VecCount(chunk->symbols.data);
  Write(file, &data_size, sizeof(u32));
  Write(file, chunk->symbols.data, data_size);

  return true;
}

bool ReadChunk(char *path, Chunk *chunk)
{
  int file = Open(path);
  if (file < 0) return false;

  u32 sig;
  Read(file, &sig, sizeof(u32));
  if (sig != 0xCA55E77E) return false;

  u32 version;
  Read(file, &version, sizeof(u32));
  if (version != 1) return false;

  u32 code_size;
  Read(file, &code_size, sizeof(u32));
  chunk->code = NewVec(u8, code_size);
  RawVecCount(chunk->code) = code_size;
  Read(file, chunk->code, code_size);

  u32 num_consts;
  Read(file, &num_consts, sizeof(u32));
  chunk->constants = NewVec(Val, num_consts);
  RawVecCount(chunk->constants) = num_consts;
  Read(file, chunk->constants, num_consts*sizeof(Val));

  u32 num_syms;
  Read(file, &num_syms, sizeof(u32));
  InitStringTable(&chunk->symbols);
  for (u32 i = 0; i < num_syms; i++) {
    u32 key, val;
    Read(file, &key, sizeof(u32));
    Read(file, &val, sizeof(u32));
    MapSet(&chunk->symbols.symbols, key, val);
  }

  chunk->symbols.positions = NewVec(u32, num_syms);
  RawVecCount(chunk->symbols.positions) = num_syms;
  Read(file, chunk->symbols.positions, sizeof(u32)*num_syms);

  u32 data_size;
  Read(file, &data_size, sizeof(u32));
  chunk->symbols.data = NewVec(u8, data_size);
  RawVecCount(chunk->symbols.data) = data_size;
  Read(file, chunk->symbols.data, data_size);

  return true;
}
