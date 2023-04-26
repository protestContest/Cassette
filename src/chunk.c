#include "chunk.h"
#include "vm.h"

static OpInfo op_info[NUM_OPCODES] = {
  [OpNoop] =    { "noop",     ArgsNone },
  [OpConst] =   { "const",    ArgsConstReg },
  [OpLookup] =  { "lookup",   ArgsConstReg },
  [OpDefine] =  { "define",   ArgsConst },
  [OpBranch] =  { "branch",   ArgsConst },
  [OpLambda] =  { "lambda",   ArgsConstReg },
  [OpDefArg] =  { "defarg",   ArgsConst },
  [OpExtEnv] =  { "extenv",   ArgsNone },
  [OpPushArg] = { "pusharg",  ArgsNone },
  [OpBrPrim] =  { "brprim",   ArgsConst },
  [OpPrim] =    { "prim",     ArgsNone },
  [OpApply] =   { "apply",    ArgsNone },
  [OpMove] =    { "move",     ArgsRegReg },
  [OpPush] =    { "push",     ArgsReg },
  [OpPop] =     { "pop",      ArgsReg },
  [OpJump] =    { "jump",     ArgsConst },
  [OpReturn] =  { "return",   ArgsNone },
  [OpHalt] =    { "halt",     ArgsNone },
};

OpInfo GetOpInfo(OpCode op)
{
  return op_info[op];
}

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
  u32 i = 0;
  i32 reg_val = RawInt(reg);
  Assert(reg_val != 0);
  while ((reg_val & 1) == 0) {
    i++;
    reg_val = reg_val >> 1;
  }
  VecPush(chunk->data, i);
}

static Val AssembleInstruction(Val stmts, Chunk *chunk, Mem *mem)
{
  Val op = Head(mem, stmts);

  for (u32 i = 0; i < NUM_OPCODES; i++) {
    if (Eq(op, SymbolFor(op_info[i].name))) {
      PushOp(i, chunk);

      switch (op_info[i].args) {
      case ArgsNone:
        return Tail(mem, stmts);
      case ArgsConst:
        PushConst(ListAt(mem, stmts, 1), chunk);
        return ListFrom(mem, stmts, 2);
      case ArgsReg:
        PushReg(ListAt(mem, stmts, 1), chunk);
        return ListFrom(mem, stmts, 2);
      case ArgsConstReg:
        PushConst(ListAt(mem, stmts, 1), chunk);
        PushReg(ListAt(mem, stmts, 2), chunk);
        return ListFrom(mem, stmts, 3);
      case ArgsRegReg:
        PushReg(ListAt(mem, stmts, 1), chunk);
        PushReg(ListAt(mem, stmts, 2), chunk);
        return ListFrom(mem, stmts, 3);
      }
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

static u8 ChunkRef(Chunk *chunk, u32 ref)
{
  Assert(ref < VecCount(chunk->data));
  return chunk->data[ref];
}

static Val GetConst(Chunk *chunk, u32 index)
{
  Assert(index < VecCount(chunk->constants));
  return chunk->constants[index];
}

static void PrintTarget(i32 reg)
{
  PrintReg(IntToReg(reg));
}

u32 PrintInstruction(Chunk *chunk, u32 i, Mem *mem)
{
  OpCode op = chunk->data[i];
  PrintOpCode(op);

  switch (op_info[op].args) {
  case ArgsNone:
    return 1;
  case ArgsConst:
    Print(" ");
    PrintVal(mem, GetConst(chunk, ChunkRef(chunk, i+1)));
    return 2;
  case ArgsReg:
    Print(" ");
    PrintTarget(ChunkRef(chunk, i+1));
    return 2;
  case ArgsConstReg:
    Print(" ");
    PrintVal(mem, GetConst(chunk, ChunkRef(chunk, i+1)));
    Print(" ");
    PrintTarget(ChunkRef(chunk, i+2));
    return 3;
  case ArgsRegReg:
    Print(" ");
    PrintTarget(ChunkRef(chunk, i+1));
    Print(" ");
    PrintTarget(ChunkRef(chunk, i+2));
    return 3;
  }
}

void PrintChunk(Chunk *chunk, Mem *mem)
{
  u32 i = 0;
  while (i < VecCount(chunk->data)) {
    PrintIntN(i, 4, ' ');
    Print("│ ");
    i += PrintInstruction(chunk, i, mem);
    Print("\n");
  }
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

void PrintOpCode(OpCode op)
{
  Print(op_info[op].name);
}
