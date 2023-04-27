#include "chunk.h"
#include "vm.h"

typedef enum {
  ArgsNone,
  ArgsConst,
  ArgsReg,
  ArgsConstReg,
  ArgsRegReg,
} ArgInfo;

typedef struct {
  char *name;
  ArgInfo args;
} OpInfo;

static OpInfo op_info[NUM_OPCODES] = {
  [OpNoop] =    { "noop",     ArgsNone },
  [OpConst] =   { "const",    ArgsConstReg },
  [OpLookup] =  { "lookup",   ArgsConstReg },
  [OpDefine] =  { "define",   ArgsConst },
  [OpBranch] =  { "branch",   ArgsConst },
  [OpNot] =     { "not",      ArgsNone },
  [OpLambda] =  { "lambda",   ArgsConstReg },
  [OpDefArg] =  { "defarg",   ArgsConst },
  [OpExtEnv] =  { "extenv",   ArgsNone },
  [OpPushArg] = { "pusharg",  ArgsNone },
  [OpBrPrim] =  { "brprim",   ArgsConst },
  [OpPrim] =    { "prim",     ArgsReg },
  [OpApply] =   { "apply",    ArgsNone },
  [OpMove] =    { "move",     ArgsRegReg },
  [OpPush] =    { "push",     ArgsReg },
  [OpPop] =     { "pop",      ArgsReg },
  [OpJump] =    { "jump",     ArgsConst },
  [OpReturn] =  { "return",   ArgsNone },
  [OpHalt] =    { "halt",     ArgsNone },
};

u32 OpLength(OpCode op)
{
  switch (op_info[op].args) {
  case ArgsNone:      return 1;
  case ArgsConst:     return 2;
  case ArgsReg:       return 2;
  case ArgsConstReg:  return 3;
  case ArgsRegReg:    return 3;
  }
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
#ifdef DEBUG_ASSEMBLE
      u32 inst_start = VecCount(chunk->data);
#endif
      PushOp(i, chunk);

#ifdef DEBUG_ASSEMBLE
      PrintVal(mem, op);
      Print(" ");
#endif

      Val result;
      switch (op_info[i].args) {
      case ArgsNone:
        result = Tail(mem, stmts);
        break;
      case ArgsConst:
#ifdef DEBUG_ASSEMBLE
        PrintVal(mem, ListAt(mem, stmts, 1));
#endif
        PushConst(ListAt(mem, stmts, 1), chunk);
        result = ListFrom(mem, stmts, 2);
        break;
      case ArgsReg:
#ifdef DEBUG_ASSEMBLE
        PrintVal(mem, ListAt(mem, stmts, 1));
#endif
        PushReg(Tail(mem, ListAt(mem, stmts, 1)), chunk);
        result = ListFrom(mem, stmts, 2);
        break;
      case ArgsConstReg:
#ifdef DEBUG_ASSEMBLE
        PrintVal(mem, ListAt(mem, stmts, 1));
        Print(" ");
        PrintVal(mem, ListAt(mem, stmts, 2));
#endif
        PushConst(ListAt(mem, stmts, 1), chunk);
        PushReg(Tail(mem, ListAt(mem, stmts, 2)), chunk);
        result = ListFrom(mem, stmts, 3);
        break;
      case ArgsRegReg:
#ifdef DEBUG_ASSEMBLE
        PrintVal(mem, ListAt(mem, stmts, 1));
        Print(" ");
        PrintVal(mem, ListAt(mem, stmts, 2));
#endif
        PushReg(Tail(mem, ListAt(mem, stmts, 1)), chunk);
        PushReg(Tail(mem, ListAt(mem, stmts, 2)), chunk);
        result = ListFrom(mem, stmts, 3);
        break;
      }

#ifdef DEBUG_ASSEMBLE
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

static u32 PrintTarget(i32 reg)
{
  return PrintReg(IntToReg(reg));
}

u32 PrintInstruction(Chunk *chunk, u32 i, Mem *mem)
{
  u32 printed = 0;

  OpCode op = chunk->data[i];
  printed += PrintOpCode(op);

  switch (op_info[op].args) {
  case ArgsNone:
    return printed;
  case ArgsConst:
    printed += Print(" ");
    printed += PrintVal(mem, ChunkConst(chunk, i+1));
    return printed;
  case ArgsReg:
    printed += Print(" ");
    printed += PrintTarget(ChunkRef(chunk, i+1));
    return printed;
  case ArgsConstReg:
    printed += Print(" ");
    printed += PrintVal(mem, ChunkConst(chunk, i+1));
    printed += Print(" ");
    printed += PrintTarget(ChunkRef(chunk, i+2));
    return printed;
  case ArgsRegReg:
    printed += Print(" ");
    printed += PrintTarget(ChunkRef(chunk, i+1));
    printed += Print(" ");
    printed += PrintTarget(ChunkRef(chunk, i+2));
    return printed;
  }
}

void PrintChunk(Chunk *chunk, Mem *mem)
{
  u32 i = 0;
  while (i < VecCount(chunk->data)) {
    PrintIntN(i, 4, ' ');
    Print("│ ");
    PrintInstruction(chunk, i, mem);
    Print("\n");
    i += OpLength(ChunkRef(chunk, i));
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

u32 PrintOpCode(OpCode op)
{
  return Print(op_info[op].name);
}
