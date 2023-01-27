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
} OpCode;

typedef enum {
  ARGS_NONE,
  ARGS_VAL,
} ArgFormat;

const char *OpStr(OpCode op);
ArgFormat OpFormat(OpCode op);
u32 OpSize(OpCode op);

Status NegOp(struct VM *vm);
Status AddOp(struct VM *vm);
Status SubOp(struct VM *vm);
Status MulOp(struct VM *vm);
Status DivOp(struct VM *vm);
