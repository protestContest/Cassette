#pragma once
#include "module.h"
#include <univ/hashmap.h>
#include <univ/vec.h>

typedef enum {
  vmOk,
  stackUnderflow,
  invalidType,
  divideByZero,
  outOfBounds,
  unhandledTrap
} VMStatus;

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
  opSlice,
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
  VMStatus status;
  u32 pc;
  val env;
  val *stack;
  Module *mod;
  PrimFn *primitives;
  HashMap primMap;
} VM;

#define CheckStack(vm, n)   if (VecCount((vm)->stack) < (n)) return stackUnderflow
#define VMDone(vm)          ((vm)->status != vmOk || (vm)->pc >= VecCount((vm)->mod->code))

void InitVM(VM *vm, Module *mod);
void DestroyVM(VM *vm);
void DefinePrimitive(val id, PrimFn fn, VM *vm);
VMStatus VMStep(VM *vm);
void VMRun(VM *vm);
val StackPop(VM *vm);
void StackPush(val value, VM *vm);

void VMTrace(VM *vm, char *src);
void TraceInst(VM *vm);
u32 PrintStack(VM *vm);
val VMError(VM *vm);
