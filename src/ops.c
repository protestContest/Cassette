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
  [OpLen]     = {"len", ArgsNone},
  [OpTrue]    = {"true", ArgsNone},
  [OpFalse]   = {"false", ArgsNone},
  [OpNil]     = {"nil", ArgsNone},
  [OpAdd]     = {"add", ArgsNone},
  [OpSub]     = {"sub", ArgsNone},
  [OpMul]     = {"mul", ArgsNone},
  [OpDiv]     = {"div", ArgsNone},
  [OpExp]     = {"exp", ArgsNone},
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
  [OpApply]   = {"apply", ArgsConst},
  [OpReturn]  = {"return", ArgsNone},
  [OpLookup]  = {"lookup", ArgsConst},
  [OpDefine]  = {"define", ArgsConst},
  [OpJump]    = {"jump", ArgsConst},
  [OpBranch]  = {"branch", ArgsConst},
  [OpBranchF] = {"branchf", ArgsConst},
  [OpPop]     = {"pop", ArgsNone},
  [OpDefMod]  = {"defmod", ArgsConst},
  [OpGetMod]  = {"getmod", ArgsConst},
  [OpExport]  = {"export", ArgsNone},
  [OpImport]  = {"import", ArgsNone},
  [OpHalt]    = {"halt", ArgsNone},
};

char *OpName(OpCode op)
{
  return ops[op].name;
}

u32 OpLength(OpCode op)
{
  switch (ops[op].args) {
  case ArgsNone:    return 1;
  case ArgsConst:   return 2;
  case ArgsReg:     return 2;
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

  switch (ops[op].args) {
  case ArgsNone:
    return length;
  case ArgsConst:
    length += Print(" ");
    length += PrintVal(ChunkConst(chunk, index+1), &chunk->constants);
    return length;
  case ArgsReg:
    length += Print(" ");
    if (ChunkRef(chunk, index+1) == RegCont) length += Print("con");
    else if (ChunkRef(chunk, index+1) == RegEnv) length += Print("env");
    else length += PrintInt(ChunkRef(chunk, index+1));
    return length;
  }
}
