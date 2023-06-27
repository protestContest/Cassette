#pragma once
#include "chunk.h"
#include "mem.h"

typedef enum {
  OpConst,
  OpStr,
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
  OpApply,
  OpReturn,
  OpLookup,
  OpDefine,
  OpJump,
  OpBranch,
  OpBranchF,
  OpPop,
} OpCode;

typedef struct {
  Chunk *chunk;
  Mem *mem;
  Val *stack;
  u32 pc;
} VM;

void InitVM(VM *vm);
void RunChunk(VM *vm, Chunk *chunk);
