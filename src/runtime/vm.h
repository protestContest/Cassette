#pragma once
#include "chunk.h"
#include "device/device.h"
#include "mem/mem.h"
#include "univ/result.h"
#include "univ/vec.h"

#define MaxDevices 32

typedef struct {
  u32 pc;
  u32 link;
  IntVec stack;
  Mem mem;
  Chunk *chunk;
  u32 dev_map;
  Device devices[MaxDevices];
  bool trace;
} VM;

#define StackPush(vm, v)    IntVecPush(&(vm)->stack, v)
#define StackPop(vm)        ((vm)->stack.items[--(vm)->stack.count])
#define StackRef(vm, i)     (vm)->stack.items[(vm)->stack.count - 1 - i]
#define Env(vm)             (vm)->stack.items[0]

void InitVM(VM *vm, Chunk *chunk);
void DestroyVM(VM *vm);
Result Run(u32 num_instructions, VM *vm);
Result RunChunk(Chunk *chunk, VM *vm);
void Halt(VM *vm);
Result RuntimeError(char *message, VM *vm);
