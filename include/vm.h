#pragma once
#include "mem.h"
#include "program.h"
#include "result.h"

/* A VM can execute bytecode in a Program. */

struct VM;
typedef Result (*PrimFn)(struct VM *vm);

typedef struct VM {
  Result status;
  u32 pc;
  val env;
  val mod;
  u32 link;
  val *stack;
  PrimFn *primitives;
  Program *program;
  void **refs;
} VM;

#define CheckStack(vm, n) \
  if (VecCount((vm)->stack) < (n)) return RuntimeError("Stack underflow", vm)
#define VMDone(vm) \
  (IsError((vm)->status) || (vm)->pc >= VecCount((vm)->program->code))

void InitVM(VM *vm, Program *program);
void DestroyVM(VM *vm);
Result VMRun(Program *program);
Result VMStep(VM *vm);
void VMStackPush(val value, VM *vm);
val VMStackPop(VM *vm);
void VMTrace(VM *vm, char *src);
u32 VMPushRef(void *ref, VM *vm);
void *VMGetRef(u32 ref, VM *vm);
void MaybeGC(u32 size, VM *vm);
void RunGC(VM *vm);
void UnwindVM(VM *vm);
Result RuntimeError(char *message, struct VM *vm);
void PrintStackTrace(Result result);
void FreeStackTrace(Result result);
