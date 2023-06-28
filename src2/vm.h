#pragma once
#include "chunk.h"
#include "mem.h"

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
  OpApply,
  OpReturn,
  OpLookup,
  OpDefine,
  OpJump,
  OpBranch,
  OpBranchF,
  OpPop,
  OpHalt,
} OpCode;

typedef struct {
  Mem mem;
  Chunk *chunk;
  Val *stack;
  u32 pc;
} VM;

void InitVM(VM *vm);
void RunChunk(VM *vm, Chunk *chunk);

u32 OpLength(OpCode op);
u32 PrintInstruction(Chunk *chunk, u32 index);
