#include "ops.h"

static struct {char *name; u32 length;} ops[] = {
  [OpHalt]      = { "halt", 1 },
  [OpPop]       = { "pop", 1 },
  [OpDup]       = { "dup", 1 },
  [OpNeg]       = { "neg", 1 },
  [OpNot]       = { "not", 1 },
  [OpLen]       = { "len", 1 },
  [OpMul]       = { "mul", 1 },
  [OpDiv]       = { "div", 1 },
  [OpRem]       = { "rem", 1 },
  [OpAdd]       = { "add", 1 },
  [OpSub]       = { "sub", 1 },
  [OpIn]        = { "in", 1 },
  [OpGt]        = { "gt", 1 },
  [OpLt]        = { "lt", 1 },
  [OpEq]        = { "eq", 1 },
  [OpConst]     = { "const", 2 }, // const num
  [OpStr]       = { "str", 1 },
  [OpPair]      = { "pair", 1 },
  [OpTuple]     = { "tuple", 2 }, // length
  [OpSet]       = { "set", 2 }, // tuple index
  [OpMap]       = { "map", 1 },
  [OpGet]       = { "access", 1 },
  [OpExtend]    = { "extend", 1 },
  [OpDefine]    = { "define", 2 }, // var index
  [OpLookup]    = { "lookup", 3 }, // frame index, var index
  [OpExport]    = { "export", 1 },
  [OpJump]      = { "jump", 2 }, // relative location
  [OpBranch]    = { "branch", 2 }, // relative location
  [OpCont]      = { "cont", 2 }, // relative location
  [OpReturn]    = { "return", 1 },
  [OpSaveEnv]   = { "save env", 1 },
  [OpRestEnv]   = { "restore env", 1 },
  [OpSaveCont]  = { "save cont", 1 },
  [OpRestCont]  = { "restore cont", 1 },
  [OpLambda]    = { "lambda", 2 }, // code size
  [OpApply]     = { "apply", 1 },
  [OpModule]    = { "module", 2 },  // module ID
  [OpLoad]      = { "load", 2 }, // module ID
};

u32 OpLength(OpCode op)
{
  return ops[op].length;
}
