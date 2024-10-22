#pragma once
#include "program.h"
#include "result.h"

/* A VM can execute bytecode in a Program. */

struct VM;
typedef Result (*PrimFn)(struct VM *vm);

enum {regEnv, regMod};

typedef struct VM {
  Result status;
  u32 regs[8];
  u32 pc;
  u32 link;
  PrimFn *primitives;
  Program *program;
  void **refs; /* vec */
} VM;

#define CheckStack(vm, n) \
  if (StackSize() < (n)) return RuntimeError("Stack underflow", vm)
#define VMDone(vm) \
  (IsError((vm)->status) || (vm)->pc >= VecCount((vm)->program->code))

void InitVM(VM *vm, Program *program);
void DestroyVM(VM *vm);
Result VMRun(Program *program);
Result VMStep(VM *vm);
void VMTrace(VM *vm, char *src);
u32 VMPushRef(void *ref, VM *vm);
void *VMGetRef(u32 ref, VM *vm);
void MaybeGC(u32 size, VM *vm);
void RunGC(VM *vm);
Result RuntimeError(char *message, struct VM *vm);
void PrintStackTrace(Result result);
void FreeStackTrace(Result result);
