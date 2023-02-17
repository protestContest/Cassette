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
  OP_DUP,

  OP_PAIR,
  OP_LIST,
  OP_DICT,
  OP_LAMBDA,

  OP_NEG,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,

  OP_NOT,
  OP_EQUAL,
  OP_GT,
  OP_LT,

  OP_DEFINE,
  OP_LOOKUP,
  OP_ACCESS,
  OP_MODULE,
  OP_IMPORT,
  OP_USE,
  OP_SCOPE,
  OP_UNSCOPE,

  OP_BRANCH,
  OP_JUMP,
  OP_APPLY,
  OP_CALL,
  OP_RETURN,
} OpCode;

typedef enum {
  ARGS_NONE,
  ARGS_VAL,
  ARGS_INT,
} ArgFormat;

char *OpStr(OpCode op);
ArgFormat OpFormat(OpCode op);
u32 OpSize(OpCode op);
void DoOp(VM *vm, OpCode op);
