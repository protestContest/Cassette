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
  [OpRem]     = {"rem", ArgsNone},
  [OpExp]     = {"exp", ArgsNone},
  [OpNeg]     = {"neg", ArgsNone},
  [OpNot]     = {"not", ArgsNone},
  [OpEq]      = {"eq", ArgsNone},
  [OpGt]      = {"gt", ArgsNone},
  [OpLt]      = {"lt", ArgsNone},
  [OpIn]      = {"in", ArgsNone},
  [OpAccess]  = {"access", ArgsNone},
  [OpLambda]  = {"lambda", ArgsLocConst},
  [OpSave]    = {"save", ArgsReg},
  [OpRestore] = {"restore", ArgsReg},
  [OpCont]    = {"cont", ArgsLoc},
  [OpApply]   = {"apply", ArgsConst},
  [OpReturn]  = {"return", ArgsNone},
  [OpLookup]  = {"lookup", ArgsConst},
  [OpDefine]  = {"define", ArgsConst},
  [OpJump]    = {"jump", ArgsConst},
  [OpBranch]  = {"branch", ArgsLoc},
  [OpBranchF] = {"branchf", ArgsLoc},
  [OpPop]     = {"pop", ArgsNone},
  [OpDefMod]  = {"defmod", ArgsConst},
  [OpGetMod]  = {"getmod", ArgsConst},
  [OpExport]  = {"export", ArgsNone},
  [OpImport]  = {"import", ArgsConst},
  [OpHalt]    = {"halt", ArgsNone},
};

void InitOps(Mem *mem)
{
  for (u32 i = 0; i < ArrayCount(ops); i++) {
    MakeSymbol(OpName(i), mem);
  }
}

char *OpName(OpCode op)
{
  return ops[op].name;
}

u32 OpLength(OpCode op)
{
  switch (ops[op].args) {
  case ArgsNone:        return 1;
  case ArgsLoc:         return 2;
  case ArgsConst:       return 2;
  case ArgsReg:         return 2;
  case ArgsLocConst:    return 3;
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
  case ArgsLoc:
    length += Print(" ");
    length += InspectVal(ChunkConst(chunk, index+1), &chunk->constants);
    return length;
  case ArgsReg:
    length += Print(" ");
    if (ChunkRef(chunk, index+1) == RegCont) length += Print("con");
    else if (ChunkRef(chunk, index+1) == RegEnv) length += Print("env");
    else length += PrintInt(ChunkRef(chunk, index+1));
    return length;
  case ArgsLocConst:
    length += Print(" ");
    length += InspectVal(ChunkConst(chunk, index+1), &chunk->constants);
    length += Print(" ");
    length += InspectVal(ChunkConst(chunk, index+2), &chunk->constants);
    return length;
  }
}

Metric *InitOpMetrics(void)
{
  Metric *metrics = NewVec(Metric, ArrayCount(ops));

  for (u32 i = 0; i < ArrayCount(ops); i++) {
    Metric m = {ops[i].name, NULL};
    VecPush(metrics, m);
  }

  return metrics;
}


