#pragma once
#include "mem.h"

typedef enum {
  opNoop,   /* */
  opConst,  /* a; -> a */
  opAdd,    /* b a -> (a+b) */
  opSub,    /* b a -> (a-b) */
  opMul,    /* b a -> (a*b) */
  opDiv,    /* b a -> (a/b) */
  opRem,    /* b a -> (a%b) */
  opAnd,    /* b a -> (a&b) */
  opOr,     /* b a -> (a|b) */
  opComp,   /* a -> (~a) */
  opLt,     /* b a -> (a<b) */
  opGt,     /* b a -> (a>b) */
  opEq,     /* b a -> (a==b) */
  opNeg,    /* a -> (-a) */
  opNot,    /* a -> (!a) */
  opShift,  /* b a -> (a<<b) */
  opNil,    /* -> nil */
  opPair,   /* b a -> pair(b,a) */
  opHead,   /* a -> head(a) */
  opTail,   /* a -> tail(a) */
  opTuple,  /* n; -> tuple(n) */
  opLen,    /* a -> len(a) */
  opGet,    /* b a -> (a[b]) */
  opSet,    /* c b a -> a; a[b] <- c */
  opStr,    /* a -> sym_to_str(a) */
  opBin,    /* n; -> bin(n) */
  opJoin,   /* b a -> (a <> b) */
  opSlice,  /* c b a -> (a[b:c]) */
  opJmp,    /* n; pc <- pc + n */
  opBranch, /* n; a -> ; pc <- pc + n */
  opTrap,   /* n; ?? */
  opPos,    /* n; -> (pc + n) */
  opGoto,   /* a -> ; pc <- pc + a */
  opHalt,   /* */
  opDup,    /* a -> a a */
  opDrop,   /* a -> */
  opSwap,   /* b a -> a b */
  opOver,   /* b a -> a b a */
  opRot,    /* c b a -> a c b */
  opGetEnv, /* -> env */
  opSetEnv, /* a -> ; env <- a */
  opLookup, /* n; -> (env[n]) */
  opDefine  /* n; a -> ; env[n] <- a */
} OpCode;

typedef enum {
  vmOk,
  stackUnderflow,
  invalidType,
  divideByZero,
  outOfBounds,
  unhandledTrap
} VMStatus;

struct VM;
typedef VMStatus (*PrimFn)(struct VM *vm);

typedef struct {
  u8 *code;
  char *symbols;
} Program;

typedef struct VM {
  VMStatus status;
  u32 pc;
  val env;
  val *stack;
  PrimFn *primitives;
  Program *program;
} VM;

#define CheckStack(vm, n) \
  if (VecCount((vm)->stack) < (n)) return stackUnderflow
#define VMDone(vm) \
  ((vm)->status != vmOk || (vm)->pc >= VecCount((vm)->program->code))

void InitVM(VM *vm, Program *program);
void DestroyVM(VM *vm);
void VMRun(Program *program, VM *vm);
VMStatus VMStep(VM *vm);
void VMStackPush(val value, VM *vm);
val VMStackPop(VM *vm);

void VMTrace(VM *vm, char *src);
