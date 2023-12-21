#pragma once
#include "mem/mem.h"
#include "chunk.h"
#include "result.h"
#include "univ/vec.h"
#include "device/device.h"

typedef struct {
  u32 pc;
  u32 link;
  bool trace;
  IntVec stack;
  Mem mem;
  Chunk *chunk;
  u32 dev_map;
  Device devices[32];
} VM;

#define StackPush(vm, v)    IntVecPush(&(vm)->stack, v)
#define StackPop(vm)        ((vm)->stack.items[--(vm)->stack.count])
#define StackRef(vm, i)     (vm)->stack.items[(vm)->stack.count - 1 - i]
#define Env(vm)             (vm)->stack.items[0]

#define IsFunction(value, mem)    (IsPair(value) && Head(value, mem) == Function)
#define IsPrimitive(value, mem)   (IsPair(value) && Head(value, mem) == Primitive)

void InitVM(VM *vm, Chunk *chunk);
void DestroyVM(VM *vm);
Result Run(VM *vm, u32 num_instructions);
Result RunChunk(Chunk *chunk, VM *vm);
Result RuntimeError(char *message, VM *vm);
bool AnyWindowsOpen(VM *vm);
