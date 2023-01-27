#pragma once

struct VM;
enum RunResult;

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

enum RunResult NegOp(struct VM *vm);
enum RunResult AddOp(struct VM *vm);
enum RunResult SubOp(struct VM *vm);
enum RunResult MulOp(struct VM *vm);
enum RunResult DivOp(struct VM *vm);
