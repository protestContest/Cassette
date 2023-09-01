#pragma once

typedef enum {
  OpHalt,
  OpPop,
  OpDup,
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
  OpConst,
  OpStr,
  OpPair,
  OpTuple,
  OpSet,
  OpMap,
  OpGet,
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
  OpLambda,
  OpApply,
  OpModule,
  OpLoad,
} OpCode;

#ifndef LIBCASSETTE
char *OpName(OpCode op);
#endif

u32 OpLength(OpCode op);
