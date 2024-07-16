#pragma once

typedef enum {
  vmOk,
  stackUnderflow,
  invalidType,
  divideByZero,
  outOfBounds,
  unhandledTrap
} VMStatus;

struct VM;
typedef VMStatus (*PrimFn)(struct VM *vm);

typedef struct {
  char *name;
  PrimFn fn;
  char *type;
} PrimDef;

PrimDef *Primitives(void);
u32 NumPrimitives(void);
