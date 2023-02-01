#pragma once

struct VM;

typedef enum {
  OP_HALT,
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
  OP_LOOKUP,
  OP_JUMP,
  OP_LAMBDA,
  OP_PAIR,
  OP_LIST,
  OP_APPLY,
} OpCode;

typedef enum {
  ARGS_NONE,
  ARGS_VAL,
  ARGS_INT,
} ArgFormat;

const char *OpStr(OpCode op);
ArgFormat OpFormat(OpCode op);
u32 OpSize(OpCode op);

void NegOp(struct VM *vm);
void ArithmeticOp(struct VM *vm, OpCode op);
void CompareOp(struct VM *vm, OpCode op);
void NotOp(struct VM *vm);
void PairOp(struct VM *vm);
void ListOp(struct VM *vm, u32 num);
void LambdaOp(struct VM *vm);
