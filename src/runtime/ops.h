#pragma once

typedef enum {OpNoop, OpHalt, OpError, OpPop, OpSwap, OpDup, OpConst, OpConst2,
  OpInt, OpNil, OpNeg, OpNot, OpBitNot, OpLen, OpMul, OpDiv, OpRem, OpAdd,
  OpSub, OpShift, OpBitAnd, OpBitOr, OpIn, OpLt, OpGt, OpEq, OpStr, OpPair,
  OpUnpair, OpTuple, OpMap, OpSet, OpGet, OpCat, OpExtend, OpDefine, OpLookup,
  OpExport, OpJump, OpBranch, OpLambda, OpLink, OpReturn, OpApply} OpCode;

u32 OpLength(OpCode op);
