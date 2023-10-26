#pragma once

typedef enum {
  OpConst,
  OpPop,
  OpNeg,
  OpNot,
  OpAnd,
  OpOr,
  OpEq,
  OpLt,
  OpGt,
  OpAdd,
  OpSub,
  OpMul,
  OpDiv,
  OpRem,
  OpLen,
  OpIn,
  OpPair,
  OpTuple,
  OpLambda,

  OpExtend,
  OpLookup,

  OpJump,
  OpLink,
  OpReturn,
  OpApply,

  NumOps
} OpCode;

char *OpName(OpCode op);
u32 OpLength(OpCode op);
