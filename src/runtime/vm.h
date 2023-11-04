#pragma once
#include "mem/mem.h"
#include "mem/symbols.h"
#include "chunk.h"

typedef struct {
  u32 pc;
  Mem stack;
  Mem mem;
  SymbolTable symbols;
} VM;

#define StackPush(vm, v)    PushMem(&(vm)->stack, v)
#define StackPop(vm)        PopMem(&(vm)->stack)
#define StackRef(vm, i)     (vm)->stack.values[(vm)->stack.count - 1 - i]
#define Env(vm)             (vm)->stack.values[0]

void InitVM(VM *vm);
void DestroyVM(VM *vm);
char *RunChunk(Chunk *chunk, VM *vm);
