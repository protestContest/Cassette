#pragma once

typedef enum {
  OpNoop,
  OpHalt,
  OpError,
  OpPop,
  OpDup,
  OpConst,
  OpConst2,
  OpInt,
  OpNil,
  OpStr,
  OpPair,
  OpTuple,
  OpMap,
  OpSet,
  OpGet,
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
