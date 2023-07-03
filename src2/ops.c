#include "ops.h"
#include "vm.h"

typedef struct {
  char *name;
  OpArgs args;
} OpInfo;

static OpInfo ops[] = {
  [OpConst]   = {"const", ArgsConst},
  [OpStr]     = {"str", ArgsNone},
  [OpPair]    = {"pair", ArgsNone},
  [OpList]    = {"list", ArgsConst},
  [OpTuple]   = {"tuple", ArgsConst},
  [OpMap]     = {"map", ArgsConst},
  [OpTrue]    = {"true", ArgsNone},
  [OpFalse]   = {"false", ArgsNone},
  [OpNil]     = {"nil", ArgsNone},
  [OpAdd]     = {"add", ArgsNone},
  [OpSub]     = {"sub", ArgsNone},
  [OpMul]     = {"mul", ArgsNone},
  [OpDiv]     = {"div", ArgsNone},
  [OpNeg]     = {"neg", ArgsNone},
  [OpNot]     = {"not", ArgsNone},
  [OpEq]      = {"eq", ArgsNone},
  [OpGt]      = {"gt", ArgsNone},
  [OpLt]      = {"lt", ArgsNone},
  [OpIn]      = {"in", ArgsNone},
  [OpAccess]  = {"access", ArgsNone},
  [OpLambda]  = {"lambda", ArgsConst},
  [OpSave]    = {"save", ArgsReg},
  [OpRestore] = {"restore", ArgsReg},
  [OpCont]    = {"cont", ArgsConst},
  [OpApply]   = {"apply", ArgsLength},
  [OpReturn]  = {"return", ArgsNone},
  [OpLookup]  = {"lookup", ArgsNone},
  [OpDefine]  = {"define", ArgsNone},
  [OpJump]    = {"jump", ArgsConst},
  [OpBranch]  = {"branch", ArgsConst},
  [OpBranchF] = {"branchf", ArgsConst},
  [OpPop]     = {"pop", ArgsNone},
  // [OpDefMod]  = {"defmod", ArgsNone},
  // [OpImport]  = {"import", ArgsNone},
  [OpHalt]    = {"halt", ArgsNone},
};

char *OpName(OpCode op)
{
  return ops[op].name;
}

u32 OpLength(OpCode op)
{
  switch (ops[op].args) {
  case ArgsNone:    return 0;
  case ArgsConst:   return 1;
  case ArgsReg:     return 1;
  case ArgsLength:  return 1;
  }
}

OpCode OpFor(Val name)
{
  for (u32 i = 0; i < ArrayCount(ops); i++) {
    if (Eq(name, SymbolFor(ops[i].name))) return i;
  }
  return -1;
}

OpArgs OpArgType(OpCode op)
{
  return ops[op].args;
}

u32 PrintInstruction(Chunk *chunk, u32 index)
{
  OpCode op = chunk->data[index];
  u32 length = Print(ops[op].name);

  switch (op) {
  case OpConst:
  case OpList:
  case OpTuple:
  case OpMap:
  case OpLambda:
  case OpCont:
    length += Print(" ");
    length += DebugVal(ChunkConst(chunk, index+1), &chunk->constants);
    break;
  case OpApply:
    length += Print(" ");
    length += PrintInt(ChunkRef(chunk, index+1));
    break;
  case OpSave:
  case OpRestore:
    length += Print(" ");
    if (ChunkRef(chunk, index+1) == RegCont) length += Print("cont");
    else if (ChunkRef(chunk, index+1) == RegEnv) length += Print("env");
    else length += PrintInt(ChunkRef(chunk, index+1));
    break;
  case OpJump:
  case OpBranch:
  case OpBranchF:
    length += Print(" ");
    length += PrintInt(RawInt(ChunkConst(chunk, index+1)));
    break;
  default: break;
  }

  return length;
}
