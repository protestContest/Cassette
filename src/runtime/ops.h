#pragma once

typedef enum {
  OpNoop,
  OpHalt,
  OpError,
  OpPop,
  OpDup,
  OpSwap,
  OpConst,
  OpConst2,
  OpInt,
  OpNil,
  OpStr,
  OpPair,
  OpTuple,
  OpSet,
  OpMap,
  OpPut,
  OpExtend,
  OpDefine,
  OpLookup,
  OpExport,
  OpJump,
  OpBranch,
  OpLambda,
  OpLink,
  OpReturn,
  OpApply
} OpCode;

u32 OpLength(OpCode op);
char *OpName(OpCode op);
