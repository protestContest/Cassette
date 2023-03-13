#pragma once
#include "../compile/chunk.h"
#include "native_map.h"

typedef enum {
  VM_Ok,
  VM_Error,
  VM_Halted,
  VM_Debug,
} VMStatus;

typedef struct VM {
  VMStatus status;
  Val *stack;
  Val *heap;
  Val env;
  Val symbols;
  u32 pc;
  NativeMap natives;
  Chunk *chunk;
} VM;

void InitVM(VM *vm);
void RunChunk(VM *vm, Chunk *chunk);

void StackPush(VM *vm, Val val);
Val StackPop(VM *vm);
Val StackPeek(VM *vm, u32 pos);
void StackTrunc(VM *vm, u32 amount);

u8 ReadByte(VM *vm);
Val ReadConst(VM *vm);

void RuntimeError(VM *vm, char *fmt);
