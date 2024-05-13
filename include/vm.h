#pragma once
#include "module.h"

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
  opNot,
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
  Module *mod;
  i32 *stack;
  i32 pc;
  i32 env;
  i32 status;
} VM;

void InitVM(VM *vm, Module *mod);
void InstantiateModule(Module *mod, VM *vm);
VMStatus VMStep(VM *vm);
void VMRun(VM *vm);
