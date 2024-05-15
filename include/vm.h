#pragma once
#include "module.h"
#include <univ.h>

typedef enum {
  ok,
  stackUnderflow,
  invalidType,
  divideByZero,
  outOfBounds,
  unhandledTrap
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
  opJoin,
  opTrunc,
  opSkip,
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

struct VM;
typedef VMStatus (*PrimFn)(struct VM *vm);

typedef struct VM {
  Module *mod;
  i32 *stack;
  u32 pc;
  i32 env;
  i32 status;
  HashMap primMap;
  PrimFn *primitives;
} VM;

#define CheckStack(vm, n)   if (VecCount((vm)->stack) < (n)) return stackUnderflow

void InitVM(VM *vm, Module *mod);
void DefinePrimitive(val id, PrimFn fn, VM *vm);
VMStatus VMStep(VM *vm);
void VMRun(VM *vm, char *src);
val StackPop(VM *vm);
void StackPush(val value, VM *vm);
void TraceInst(VM *vm);
u32 PrintStack(VM *vm);
val VMError(VM *vm);
