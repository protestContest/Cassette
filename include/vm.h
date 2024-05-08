#pragma once
#include "program.h"

typedef enum {
  ok,
  stackUnderflow,
  invalidType,
  divideByZero,
  outOfBounds,
  undefined
} VMStatus;

enum {pc, env};

typedef enum {
  opNoop,
  opConst,
  opAdd,
  opSub,
  opMul,
  opDiv,
  opRem,
  opAnd,
  opOr,
  opComp,
  opLt,
  opGt,
  opEq,
  opNeg,
  opShift,
  opNil,
  opPair,
  opHead,
  opTail,
  opTuple,
  opLen,
  opGet,
  opSet,
  opStr,
  opBin,
  opJmp,
  opBr,
  opTrap,
  opPos,
  opGoto,
  opHalt,
  opDup,
  opDrop,
  opSwap,
  opOver,
  opRot,
  opGetEnv,
  opSetEnv,
  opLookup,
  opDefine
} OpCode;

typedef struct {
  Program *program;
  i32 *stack;
  i32 pc;
  i32 env;
  i32 status;
} VM;

void InitVM(VM *vm, Program *program);
void VMStep(VM *vm, i32 count);
void VMRun(VM *vm);
