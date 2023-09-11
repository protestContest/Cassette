#pragma once
#include "heap.h"
#include "chunk.h"
#include "univ/hashmap.h"

typedef enum {
  NoError,
  StackError,
  TypeError,
  ArithmeticError,
  EnvError,
  KeyError,
  ArityError,
  RuntimeError
} VMError;

#define StackMax 1024
#define CallStackMax 1024
#define ModuleMax 256

typedef struct {
  u32 pc;
  u32 cont;
  Val env;
  u32 stack_count;
  Val stack[StackMax];
  u32 call_stack_count;
  Val call_stack[CallStackMax];
  u32 mod_count;
  Val modules[ModuleMax];
  HashMap mod_map;
  Heap mem;
  VMError error;
  CassetteOpts *opts;
} VM;

typedef enum {
  RegCont,
  RegEnv,
} Reg;

void InitVM(VM *vm, CassetteOpts *opts);
void DestroyVM(VM *vm);
void RunChunk(VM *vm, Chunk *chunk);

void StackPush(VM *vm, Val value);
Val StackPop(VM *vm);
