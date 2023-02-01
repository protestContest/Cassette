#pragma once

typedef struct VM VM;

typedef enum {
  OP_HALT,
  OP_BREAK,

  OP_CONST,
  OP_TRUE,
  OP_FALSE,
  OP_NIL,
  OP_ZERO,
  OP_POP,

  OP_PAIR,
  OP_LIST,
  OP_LAMBDA,

  OP_NEG,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_EXP,

  OP_NOT,
  OP_EQUAL,
  OP_GT,
  OP_LT,

  OP_DEFINE,
  OP_LOOKUP,

  OP_BRANCH,
  OP_JUMP,
  OP_RETURN,
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

void DoOp(VM *vm, OpCode op);
