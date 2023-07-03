#pragma once
#include "chunk.h"

typedef enum {
  OpConst,
  OpStr,
  OpPair,
  OpList,
  OpTuple,
  OpMap,
  OpTrue,
  OpFalse,
  OpNil,
  OpAdd,
  OpSub,
  OpMul,
  OpDiv,
  OpNeg,
  OpNot,
  OpEq,
  OpGt,
  OpLt,
  OpIn,
  OpAccess,
  OpLambda,
  OpSave,
  OpRestore,
  OpCont,
  OpApply,
  OpReturn,
  OpLookup,
  OpDefine,
  OpJump,
  OpBranch,
  OpBranchF,
  OpPop,
  // OpDefMod,
  // OpImport,
  OpHalt,
} OpCode;

typedef enum {
  ArgsNone,
  ArgsConst,
  ArgsReg,
  ArgsLength,
} OpArgs;

char *OpName(OpCode op);
u32 OpLength(OpCode op);
OpCode OpFor(Val symbol);
OpArgs OpArgType(OpCode op);
u32 PrintInstruction(Chunk *chunk, u32 index);
