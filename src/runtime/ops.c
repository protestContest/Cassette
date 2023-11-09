#include "ops.h"

static u8 op_lengths[] = {
  [OpNoop]    = 1,
  [OpHalt]    = 1,
  [OpError]   = 1,
  [OpPop]     = 1,
  [OpDup]     = 1,
  [OpConst]   = 2,
  [OpNeg]     = 1,
  [OpNot]     = 1,
  [OpBitNot]  = 1,
  [OpLen]     = 1,
  [OpMul]     = 1,
  [OpDiv]     = 1,
  [OpRem]     = 1,
  [OpAdd]     = 1,
  [OpSub]     = 1,
  [OpShift]   = 1,
  [OpBitAnd]  = 1,
  [OpBitOr]   = 1,
  [OpIn]      = 1,
  [OpLt]      = 1,
  [OpGt]      = 1,
  [OpEq]      = 1,
  [OpStr]     = 1,
  [OpPair]    = 1,
  [OpTuple]   = 1,
  [OpSet]     = 2,
  [OpGet]     = 1,
  [OpCat]     = 1,
  [OpExtend]  = 1,
  [OpDefine]  = 2,
  [OpLookup]  = 3,
  [OpExport]  = 1,
  [OpJump]    = 2,
  [OpBranch]  = 2,
  [OpLink]    = 2,
  [OpReturn]  = 1,
  [OpApply]   = 2,
};

u32 OpLength(OpCode op)
{
  return op_lengths[op];
}
