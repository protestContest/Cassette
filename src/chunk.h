#pragma once
#include "mem.h"

typedef enum {
  ArgsNone,
  ArgsConst,
  ArgsReg,
  ArgsConstReg,
  ArgsRegReg,
} ArgInfo;

typedef struct {
  char *name;
  ArgInfo args;
} OpInfo;

typedef enum {
  OpNoop,
  OpConst,
  OpLookup,
  OpDefine,
  OpBranch,
  OpLambda,
  OpDefArg,
  OpExtEnv,
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

typedef struct {
  u8 *data;
  Val *constants;
} Chunk;

OpInfo GetOpInfo(OpCode op);

Chunk Assemble(Val stmts, Mem *mem);
void PrintChunk(Chunk *chunk, Mem *mem);
void PrintChunkConstants(Chunk *chunk, Mem *mem);

void PrintOpCode(OpCode op);
