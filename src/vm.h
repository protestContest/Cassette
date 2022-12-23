#pragma once

#define NUM_SYMBOLS 256
#define MEMORY_SIZE 4096

typedef struct {
  u32 hash;
  char *name;
} Symbol;

typedef struct {
  u32 exp;
  u32 env;
  u32 proc;
  u32 argl;
  u32 val;
  u32 cont;
  u32 unev;
  u32 sp;
  u32 base;
  u32 free;
  Symbol symbols[NUM_SYMBOLS];
  u32 mem[MEMORY_SIZE];
} VM;

void InitVM(VM *vm);
void StackPush(VM *vm, u32 val);
u32 StackPop(VM *vm);
u32 MakeSymbol(VM *vm, char *src);
u32 MakePair(VM *vm, u32 head, u32 tail);
u32 Head(VM *vm, u32 pair);
u32 Tail(VM *vm, u32 pair);
void SetHead(VM *vm, u32 pair, u32 head);
void SetTail(VM *vm, u32 pair, u32 tail);
u32 MakeTuple(VM *vm, u32 length, ...);
u32 TupleAt(VM *vm, u32 tuple, u32 i);

void DumpVM(VM *vm);
