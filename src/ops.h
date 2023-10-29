#pragma once

typedef enum {
  RegEnv = 0x1,
  RegRet = 0x2
} Reg;

typedef enum {
  OpHalt,
  OpPop,
  OpDup,
  OpConst,
  OpNeg,
  OpNot,
  OpLen,
  OpMul,
  OpDiv,
  OpRem,
  OpAdd,
  OpSub,
  OpIn,
  OpLt,
  OpGt,
  OpEq,
  OpStr,
  OpPair,
  OpTuple,
  OpSet,
  OpGet,
  OpExtend,
  OpDefine,
  OpLookup,
  OpExport,
  OpJump,
  OpBranch,
  OpLink,
  OpReturn,
  OpLambda,
  OpApply,
  OpSaveEnv,
  OpRestEnv,
  OpSaveRet,
  OpRestRet,

  NumOps
} OpCode;

char *OpName(OpCode op);
u32 OpLength(OpCode op);
