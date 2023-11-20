#pragma once
#include "mem/mem.h"
#include "chunk.h"
#include "result.h"
#include "univ/vec.h"

typedef struct {
  u32 pc;
  IntVec stack;
  Mem mem;
  Chunk *chunk;
  ObjVec canvases;
} VM;

#define StackPush(vm, v)    IntVecPush(&(vm)->stack, v)
#define StackPop(vm)        ((vm)->stack.items[--(vm)->stack.count])
#define StackRef(vm, i)     (vm)->stack.items[(vm)->stack.count - 1 - i]
#define Env(vm)             (vm)->stack.items[0]

void InitVM(VM *vm, Chunk *chunk);
void DestroyVM(VM *vm);
Result Run(VM *vm, u32 num_instructions);
Result RunChunk(Chunk *chunk, VM *vm);
Result RuntimeError(char *message, VM *vm);