#include "ops.h"
#include "vm.h"

typedef struct {
  char *name;
  u32 length;
} OpInfo;

static OpInfo ops[] = {
  [OpConst]   = {"const", 2},
  [OpStr]     = {"str", 1},
  [OpPair]    = {"pair", 1},
  [OpList]    = {"list", 2},
  [OpTuple]   = {"tuple", 2},
  [OpMap]     = {"map", 2},
  [OpTrue]    = {"true", 1},
  [OpFalse]   = {"false", 1},
  [OpNil]     = {"nil", 1},
  [OpAdd]     = {"add", 1},
  [OpSub]     = {"sub", 1},
  [OpMul]     = {"mul", 1},
  [OpDiv]     = {"div", 1},
  [OpNeg]     = {"neg", 1},
  [OpNot]     = {"not", 1},
  [OpEq]      = {"eq", 1},
  [OpGt]      = {"gt", 1},
  [OpLt]      = {"lt", 1},
  [OpIn]      = {"in", 1},
  [OpAccess]  = {"access", 1},
  [OpLambda]  = {"lambda", 2},
  [OpSave]    = {"save", 2},
  [OpRestore] = {"restore", 2},
  [OpCont]    = {"cont", 2},
  [OpApply]   = {"apply", 2},
  [OpReturn]  = {"return", 1},
  [OpLookup]  = {"lookup", 1},
  [OpDefine]  = {"define", 1},
  [OpJump]    = {"jump", 2},
  [OpBranch]  = {"branch", 2},
  [OpBranchF] = {"branchf", 2},
  [OpPop]     = {"pop", 1},
  // [OpDefMod]  = {"defmod", 1},
  // [OpImport]  = {"import", 1},
  [OpHalt]    = {"halt", 1},
};

u32 OpLength(OpCode op)
{
  return ops[op].length;
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
    length += Print(" ");
    length += DebugVal(ChunkConst(chunk, index+1), &chunk->constants);
    break;
  case OpLambda:
  case OpCont:
    length += Print(" ");
    length += PrintInt(RawInt(ChunkConst(chunk, index+1)));
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
