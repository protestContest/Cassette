#pragma once

typedef enum {OpNoop, OpHalt, OpError, OpPop, OpDup, OpConst, OpNeg, OpNot,
  OpBitNot, OpLen, OpMul, OpDiv, OpRem, OpAdd, OpSub, OpShift, OpBitAnd,
  OpBitOr, OpIn, OpLt, OpGt, OpEq, OpStr, OpPair, OpTuple, OpMap, OpSet, OpGet,
  OpCat, OpExtend, OpDefine, OpLookup, OpExport, OpJump, OpBranch, OpLink,
  OpReturn, OpApply} OpCode;

u32 OpLength(OpCode op);