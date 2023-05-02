#pragma once
#include "mem.h"

typedef enum {
  OpNoop,
  OpConst,
  OpLookup,
  OpDefine,
  OpBranch,
  OpNot,
  OpLambda,
  OpDefArg,
  OpExtEnv,
  OpImport,
  OpPushArg,
  OpBrPrim,
  OpPrim,
  OpApply,
  OpMove,
  OpPush,
  OpPop,
  OpJump,
  OpReturn,
  OpHalt,

  NUM_OPCODES
} OpCode;

typedef enum {
  ArgsNone,
  ArgsConst,
  ArgsReg,
  ArgsConstReg,
  ArgsRegReg,
} ArgInfo;

char *OpName(OpCode op);
ArgInfo OpArgs(OpCode op);
u32 OpLength(OpCode op);
void InitOps(Mem *mem);
u32 PrintOpCode(OpCode op);
