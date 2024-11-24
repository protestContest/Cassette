#pragma once
#include "runtime/error.h"
#include "runtime/program.h"

/* A VM can execute bytecode in a Program. */

struct VM;

enum {regEnv, regMod};

typedef struct VM {
  Error *error;
  u32 regs[8];
  u32 pc;
  u32 link;
  Program *program;
  void **refs; /* vec */
} VM;

void InitVM(VM *vm, Program *program);
void DestroyVM(VM *vm);
void VMStep(VM *vm);
Error *VMRun(Program *program);
u32 VMPushRef(void *ref, VM *vm);
void *VMGetRef(u32 ref, VM *vm);
i32 VMFindRef(void *ref, VM *vm);
void MaybeGC(u32 size, VM *vm);
u32 RuntimeError(char *message, VM *vm);
