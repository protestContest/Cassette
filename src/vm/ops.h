#pragma once

typedef enum {
  OpHalt,
  OpPop,
  OpDup,
  OpConst,
  OpAccess,
  OpNeg,
  OpNot,
  OpLen,
  OpMul,
  OpDiv,
  OpRem,
  OpAdd,
  OpSub,
  OpIn,
  OpGt,
  OpLt,
  OpEq,
  OpGet,
  OpPair,
  OpHead,
  OpTail,
  OpTuple,
  OpSet,
  OpMap,
  OpLambda,
  OpExtend,
  OpDefine,
  OpLookup,
  OpExport,
  OpJump,
  OpBranch,
  OpCont,
  OpReturn,
  OpSaveEnv,
  OpRestEnv,
  OpSaveCont,
  OpRestCont,
  OpApply,
} OpCode;

char *OpName(OpCode op);
u32 OpLength(OpCode op);