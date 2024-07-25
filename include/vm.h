#pragma once
#include "result.h"
#include "program.h"
#include "ops.h"
#include "mem.h"

/* A VM can execute bytecode in a Program. */

struct VM;
typedef Result (*PrimFn)(struct VM *vm);

typedef struct VM {
  Result status;
  u32 pc;
  val env;
  val mod;
  val *stack;
  PrimFn *primitives;
  Program *program;
} VM;

#define CheckStack(vm, n) \
  if (VecCount((vm)->stack) < (n)) return RuntimeError("Stack underflow", vm)
#define VMDone(vm) \
  (IsError((vm)->status) || (vm)->pc >= VecCount((vm)->program->code))

void InitVM(VM *vm, Program *program);
void DestroyVM(VM *vm);
Result VMRun(Program *program);
Result VMDebug(Program *program);
Result VMStep(VM *vm);
void VMStackPush(val value, VM *vm);
val VMStackPop(VM *vm);
void VMTrace(VM *vm, char *src);
