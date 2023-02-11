#pragma once
#include "value.h"
#include "chunk.h"
#include "native.h"

typedef enum {
  VM_Ok,
  VM_Error,
  VM_Halted,
  VM_Debug,
} VMStatus;

typedef struct VM {
  VMStatus status;
  u32 pc;
  Chunk *chunk;
  Val *stack;
  Val *heap;
  Val env;
  Val modules;
  NativeMap natives;
} VM;

void InitVM(VM *vm);
void ResetVM(VM *vm);
void DebugVM(VM *vm);

void StackPush(VM *vm, Val val);
Val StackPop(VM *vm);
Val StackPeek(VM *vm, u32 pos);
void StackTrunc(VM *vm, u32 amount);
u8 ReadByte(VM *vm);
Val ReadConst(VM *vm);

void RunChunk(VM *vm, Chunk *chunk);
void Interpret(VM *vm, char *src);
void RuntimeError(VM *vm, char *fmt, ...);

u32 PrintVMVal(VM *vm, Val val);

Val GetModule(VM *vm, Val name);
void PutModule(VM *vm, Val name, Val mod);
