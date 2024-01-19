#pragma once

/* The VM has these registers: the program counter (pc), link, stack, and
memory. The link register is the stack position of the previous link register
value, which is updated with the link instruction. This is used before function
calls to save the return address; with the link register, we can construct a
stacktrace. The VM also stores open device contexts in the devices array, and
uses the dev_map as a bitfield to track which devices are open. The trace flag
controls whether instruction tracing is enabled. */

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
#define StackDrop(vm, n)    ((vm)->stack.count -= (n))
#define StackRef(vm, i)     (vm)->stack.items[(vm)->stack.count - 1 - i]
#define Env(vm)             (vm)->stack.items[0]

void InitVM(VM *vm, Chunk *chunk);
void DestroyVM(VM *vm);
Result Run(u32 num_instructions, VM *vm);
Result RunChunk(Chunk *chunk, VM *vm);
void Halt(VM *vm);
Result RuntimeError(char *message, VM *vm);
