#pragma once
#include "chunk.h"

typedef enum {
  OpConst,
  OpStr,
  OpPair,
  OpList,
  OpTuple,
  OpMap,
  OpLen,
  OpTrue,
  OpFalse,
  OpNil,
  OpAdd,
  OpSub,
  OpMul,
  OpDiv,
  OpExp,
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
  OpHalt,
  OpDefMod,
  OpGetMod,
  OpExport,
  OpImport,
} OpCode;

typedef enum {
  ArgsNone,
  ArgsConst,
  ArgsReg,
  ArgsConstConst,
} OpArgs;

#define OpSymbol(op)  SymbolFor(OpName(op))

void InitOps(Mem *mem);
char *OpName(OpCode op);
u32 OpLength(OpCode op);
OpCode OpFor(Val symbol);
OpArgs OpArgType(OpCode op);
u32 PrintInstruction(Chunk *chunk, u32 index);

