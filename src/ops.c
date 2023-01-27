#include "ops.h"
#include "vec.h"
#include "value.h"
#include "vm.h"

typedef struct {
  char *name;
  ArgFormat format;
} OpInfo;

static OpInfo ops[] = {
  [OP_RETURN] =   { "return", ARGS_NONE     },
  [OP_CONST] =    { "const",  ARGS_VAL      },
  [OP_NEG] =      { "neg", ARGS_NONE        },
  [OP_ADD] =      { "add", ARGS_NONE        },
  [OP_SUB] =      { "sub", ARGS_NONE        },
  [OP_MUL] =      { "mul", ARGS_NONE        },
  [OP_DIV] =      { "div", ARGS_NONE        },
};

const char *OpStr(OpCode op)
{
  return ops[op].name;
}

ArgFormat OpFormat(OpCode op)
{
  return ops[op].format;
}

u32 OpSize(OpCode op)
{
  switch (OpFormat(op)) {
  case ARGS_NONE:     return 1;
  case ARGS_VAL:      return 2;
  }
}

Status NegOp(struct VM *vm)
{
  Val val = VecPop(vm->stack);
  if (IsNum(val)) {
    VecPush(vm->stack, NumVal(-val.as_f));
    return Ok;
  } else if (IsInt(val)) {
    VecPush(vm->stack, IntVal(-RawInt(val)));
    return Ok;
  } else {
    return RuntimeError(vm, "Arithmetic error");
  }
}

Status AddOp(struct VM *vm)
{
  Val b = VecPop(vm->stack);
  Val a = VecPop(vm->stack);

  if (!IsNumeric(a) || !IsNumeric(b)) return RuntimeError(vm, "Arithmetic error");

  if (IsInt(a) && IsInt(b)) VecPush(vm->stack, IntVal(RawInt(a) + RawInt(b)));
  if (IsNum(a) && IsNum(b)) VecPush(vm->stack, NumVal(a.as_f + b.as_f));
  if (IsNum(a) && IsInt(b)) VecPush(vm->stack, NumVal(a.as_f + (float)RawInt(b)));
  if (IsInt(a) && IsNum(b)) VecPush(vm->stack, NumVal((float)RawInt(a) + b.as_f));

  return Ok;
}

Status SubOp(struct VM *vm)
{
  Val b = VecPop(vm->stack);
  Val a = VecPop(vm->stack);

  if (!IsNumeric(a) || !IsNumeric(b)) return RuntimeError(vm, "Arithmetic error");

  if (IsInt(a) && IsInt(b)) VecPush(vm->stack, IntVal(RawInt(a) - RawInt(b)));
  if (IsNum(a) && IsNum(b)) VecPush(vm->stack, NumVal(a.as_f - b.as_f));
  if (IsNum(a) && IsInt(b)) VecPush(vm->stack, NumVal(a.as_f - (float)RawInt(b)));
  if (IsInt(a) && IsNum(b)) VecPush(vm->stack, NumVal((float)RawInt(a) - b.as_f));

  return Ok;
}

Status MulOp(struct VM *vm)
{
  Val b = VecPop(vm->stack);
  Val a = VecPop(vm->stack);

  if (!IsNumeric(a) || !IsNumeric(b)) return RuntimeError(vm, "Arithmetic error");

  if (IsInt(a) && IsInt(b)) VecPush(vm->stack, NumVal(RawInt(a) * RawInt(b)));
  if (IsNum(a) && IsNum(b)) VecPush(vm->stack, NumVal(a.as_f * b.as_f));
  if (IsNum(a) && IsInt(b)) VecPush(vm->stack, NumVal(a.as_f * (float)RawInt(b)));
  if (IsInt(a) && IsNum(b)) VecPush(vm->stack, NumVal((float)RawInt(a) * b.as_f));

  return Ok;
}

Status DivOp(struct VM *vm)
{
  Val b = VecPop(vm->stack);
  Val a = VecPop(vm->stack);

  if (!IsNumeric(a) || !IsNumeric(b)) return RuntimeError(vm, "Arithmetic error");
  if ((IsInt(b) && RawInt(b) == 0) || (IsNum(b) && b.as_f == 0.0)) return RuntimeError(vm, "Divide by zero");

  if (IsInt(a) && IsInt(b)) VecPush(vm->stack, NumVal((float)RawInt(a) / RawInt(b)));
  if (IsNum(a) && IsNum(b)) VecPush(vm->stack, NumVal(a.as_f / b.as_f));
  if (IsNum(a) && IsInt(b)) VecPush(vm->stack, NumVal(a.as_f / (float)RawInt(b)));
  if (IsInt(a) && IsNum(b)) VecPush(vm->stack, NumVal((float)RawInt(a) / b.as_f));

  return Ok;
}
