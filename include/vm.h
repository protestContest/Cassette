#pragma once
#include "module.h"
#include "primitives.h"
#include <univ/hashmap.h>
#include <univ/vec.h>

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
  opBr,     /* n; a -> ; pc <- pc + n */
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
  opGetCont,
  opSetCont,
  opLookup, /* n; -> (env[n]) */
  opDefine  /* n; a -> ; env[n] <- a */
} OpCode;

typedef struct VM {
  VMStatus status;
  u32 pc;
  val env;
  val cont;
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
