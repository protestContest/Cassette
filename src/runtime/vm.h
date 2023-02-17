#pragma once
#include "../image/module.h"
#include "../image/image.h"
#include "native_map.h"

typedef enum {
  VM_Ok,
  VM_Error,
  VM_Halted,
  VM_Debug,
} VMStatus;

typedef struct VM {
  VMStatus status;
  Image *image;
  Val *stack;
  Module *module;
  u32 pc;
  NativeMap natives;
} VM;

void InitVM(VM *vm);
void RunImage(VM *vm, Image *image);

void StackPush(VM *vm, Val val);
Val StackPop(VM *vm);
Val StackPeek(VM *vm, u32 pos);
void StackTrunc(VM *vm, u32 amount);

u8 ReadByte(VM *vm);
Val ReadConst(VM *vm);

void RuntimeError(VM *vm, char *fmt);



// void ResetVM(VM *vm);
// void DebugVM(VM *vm);


// void RunChunk(VM *vm, Chunk *chunk);
// void Interpret(VM *vm, char *src);

// u32 PrintVMVal(VM *vm, Val val);

// Val GetModuleEnv(VM *vm, Val name);
// void PutModule(VM *vm, Val name, Val env);
