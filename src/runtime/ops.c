#include "ops.h"

static u8 op_lengths[] = {
  [OpNoop]    = 1,
  [OpHalt]    = 1,
  [OpError]   = 1,
  [OpPop]     = 1,
  [OpDup]     = 1,
  [OpConst]   = 2,
  [OpConst2]  = 3,
  [OpInt]     = 2,
  [OpNil]     = 1,
  [OpStr]     = 1,
  [OpPair]    = 1,
  [OpTuple]   = 1,
  [OpMap]     = 1,
  [OpSet]     = 1,
  [OpGet]     = 1,
  [OpExtend]  = 1,
  [OpDefine]  = 1,
  [OpLookup]  = 1,
  [OpExport]  = 1,
  [OpJump]    = 1,
  [OpBranch]  = 1,
  [OpLambda]  = 1,
  [OpLink]    = 1,
  [OpReturn]  = 1,
  [OpApply]   = 2,
};

u32 OpLength(OpCode op)
{
  return op_lengths[op];
}
