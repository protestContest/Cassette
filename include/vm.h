#pragma once
#include "program.h"
#include "error.h"
#include "univ/vec.h"

/* A VM can execute bytecode in a Program. */

struct VM;
typedef u32 (*PrimFn)(struct VM *vm);

enum {regEnv, regMod};

typedef struct VM {
  Error *error;
  u32 regs[8];
  u32 pc;
  u32 link;
  PrimFn *primitives;
  Program *program;
  VecOf(void*) **refs;
} VM;

typedef struct StackTrace {
  char *filename;
  u32 pos;
  struct StackTrace *next;
} StackTrace;

void InitVM(VM *vm, Program *program);
void DestroyVM(VM *vm);
Error *VMRun(Program *program);
void VMStep(VM *vm);
void VMTrace(VM *vm, char *src);
u32 VMPushRef(void *ref, VM *vm);
void *VMGetRef(u32 ref, VM *vm);
i32 VMFindRef(void *ref, VM *vm);
void MaybeGC(u32 size, VM *vm);
void RunGC(VM *vm);
u32 RuntimeError(char *message, struct VM *vm);
void PrintStackTrace(StackTrace *st);
void FreeStackTrace(StackTrace *st);
