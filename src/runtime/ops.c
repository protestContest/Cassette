#include "ops.h"

typedef struct {
  u32 length;
  char *name;
} OpInfo;

static OpInfo ops[] = {
  [OpNoop]    = {1, "noop"},
  [OpHalt]    = {1, "halt"},
  [OpError]   = {1, "error"},
  [OpPop]     = {1, "pop"},
  [OpDup]     = {1, "dup"},
  [OpConst]   = {2, "const"},
  [OpNeg]     = {1, "neg"},
  [OpNot]     = {1, "not"},
  [OpMul]     = {1, "mul"},
  [OpDiv]     = {1, "div"},
  [OpRem]     = {1, "rem"},
  [OpAdd]     = {1, "add"},
  [OpSub]     = {1, "sub"},
  [OpIn]      = {1, "in"},
  [OpLt]      = {1, "lt"},
  [OpGt]      = {1, "gt"},
  [OpEq]      = {1, "eq"},
  [OpStr]     = {1, "str"},
  [OpPair]    = {1, "pair"},
  [OpTuple]   = {1, "tuple"},
  [OpSet]     = {2, "set"},
  [OpGet]     = {2, "get"},
  [OpExtend]  = {1, "extend"},
  [OpDefine]  = {2, "define"},
  [OpLookup]  = {3, "lookup"},
  [OpExport]  = {1, "export"},
  [OpJump]    = {2, "jump"},
  [OpBranch]  = {2, "branch"},
  [OpLink]    = {2, "link"},
  [OpReturn]  = {1, "return"},
  [OpLambda]  = {2, "lambda"},
  [OpApply]   = {2, "apply"}
};

char *OpName(OpCode op)
{
  return ops[op].name;
}

u32 OpLength(OpCode op)
{
  return ops[op].length;
}
