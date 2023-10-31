#pragma once
#include "mem.h"
#include "symbols.h"
#include "chunk.h"

typedef enum {
  RegEnv = 0x1,
  RegRet = 0x2
} Reg;

typedef enum {
  OpNoop,
  OpHalt,
  OpPop,
  OpDup,
  OpConst,
  OpNeg,
  OpNot,
  OpLen,
  OpMul,
  OpDiv,
  OpRem,
  OpAdd,
  OpSub,
  OpIn,
  OpLt,
  OpGt,
  OpEq,
  OpStr,
  OpPair,
  OpTuple,
  OpSet,
  OpGet,
  OpExtend,
  OpDefine,
  OpLookup,
  OpExport,
  OpJump,
  OpBranch,
  OpLink,
  OpReturn,
  OpLambda,
  OpApply,

  NumOps
} OpCode;

char *OpName(OpCode op);
u32 OpLength(OpCode op);

typedef struct {
  u32 pc;
  Val env;
  struct {
    u32 count;
    u32 capacity;
    Val *data;
  } stack;
  Mem mem;
  SymbolTable symbols;
} VM;

#define StackRef(vm, i)     (vm)->stack.data[(vm)->stack.count - 1 - i]

void InitVM(VM *vm);
void DestroyVM(VM *vm);

char *RunChunk(Chunk *chunk, VM *vm);
