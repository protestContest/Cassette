#include "ops.h"

typedef struct {
  char *name;
  u32 length;
} OpInfo;

static OpInfo ops[NumOps] = {
  [OpConst] = {"const", 2},
  [OpPop] = {"pop", 1},
  [OpNeg] = {"neg", 1},
  [OpNot] = {"not", 1},
  [OpEq] = {"eq", 1},
  [OpAdd] = {"add", 1},
  [OpSub] = {"sub", 1},
  [OpMul] = {"mul", 1},
  [OpDiv] = {"div", 1},
  [OpRem] = {"rem", 1},
  [OpTuple] = {"tuple", 1},
  [OpExtend] = {"extend", 1},
  [OpLookup] = {"lookup", 3},
  [OpLambda] = {"lambda", 1},
  [OpJump] = {"jump", 1},
  [OpLink] = {"link", 1},
  [OpReturn] = {"return", 1},
  [OpApply] = {"apply", 1}
};

char *OpName(OpCode op)
{
  return ops[op].name;
}

u32 OpLength(OpCode op)
{
  return ops[op].length;
}
