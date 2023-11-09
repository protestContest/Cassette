#pragma once

typedef enum {OpNoop, OpHalt, OpError, OpPop, OpDup, OpConst, OpNeg, OpNot,
  OpLen, OpMul, OpDiv, OpRem, OpAdd, OpSub, OpIn, OpLt, OpGt, OpEq, OpStr,
  OpPair, OpTuple, OpSet, OpGet, OpCat, OpExtend, OpDefine, OpLookup, OpExport,
  OpJump, OpBranch, OpLink, OpReturn, OpApply} OpCode;

u32 OpLength(OpCode op);
