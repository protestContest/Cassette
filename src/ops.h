#pragma once

struct VM;

typedef enum {
  OP_RETURN,
  OP_CONST,
  OP_NEG,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_EXP,
  OP_TRUE,
  OP_FALSE,
  OP_NIL,
  OP_NOT,
  OP_EQUAL,
  OP_GT,
  OP_LT,
  OP_CONS,
} OpCode;

typedef enum {
  ARGS_NONE,
  ARGS_VAL,
} ArgFormat;

const char *OpStr(OpCode op);
ArgFormat OpFormat(OpCode op);
u32 OpSize(OpCode op);

void NegOp(struct VM *vm);
void ArithmeticOp(struct VM *vm, OpCode op);
void NotOp(struct VM *vm);
void ConsOp(struct VM *vm);