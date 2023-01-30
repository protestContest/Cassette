#include "ops.h"
#include "vec.h"
#include "value.h"
#include "vm.h"
#include "mem.h"

typedef struct {
  char *name;
  ArgFormat format;
} OpInfo;

static OpInfo ops[] = {
  [OP_RETURN] =   { "return", ARGS_NONE   },
  [OP_CONST] =    { "const",  ARGS_VAL    },
  [OP_NEG] =      { "neg",    ARGS_NONE   },
  [OP_ADD] =      { "add",    ARGS_NONE   },
  [OP_SUB] =      { "sub",    ARGS_NONE   },
  [OP_MUL] =      { "mul",    ARGS_NONE   },
  [OP_DIV] =      { "div",    ARGS_NONE   },
  [OP_EXP] =      { "exp",    ARGS_NONE   },
  [OP_TRUE] =     { "true",   ARGS_NONE   },
  [OP_FALSE] =    { "false",  ARGS_NONE   },
  [OP_NIL] =      { "nil",    ARGS_NONE   },
  [OP_NOT] =      { "not",    ARGS_NONE   },
  [OP_EQUAL] =    { "equal",  ARGS_NONE   },
  [OP_GT] =       { "gt",     ARGS_NONE   },
  [OP_LT] =       { "lt",     ARGS_NONE   },
  [OP_CONS] =     { "cons",   ARGS_NONE   },
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

static bool IsFalsey(Val value)
{
  return !IsNil(value) && !Eq(value, SymbolFor("false"));
}

void NegOp(struct VM *vm)
{
  Val val = VecPop(vm->stack);
  if (IsNum(val)) {
    VecPush(vm->stack, NumVal(-val.as_f));
  } else if (IsInt(val)) {
    VecPush(vm->stack, IntVal(-RawInt(val)));
  } else {
    RuntimeError(vm, "Arithmetic error");
  }
}

void ArithmeticOp(struct VM *vm, OpCode op)
{
  Val b = VecPop(vm->stack);
  Val a = VecPop(vm->stack);

  if (!IsNumeric(a) || !IsNumeric(b)) {
    RuntimeError(vm, "Arithmetic error");
    return;
  }

  if (IsInt(a) && IsInt(b)) {
    if (op == OP_ADD) {
      VecPush(vm->stack, IntVal(RawInt(a) + RawInt(b)));
      return;
    } else if (op == OP_SUB) {
      VecPush(vm->stack, IntVal(RawInt(a) - RawInt(b)));
      return;
    }
  }

  float a_f = IsNum(a) ? a.as_f : (float)RawInt(a);
  float b_f = IsNum(b) ? b.as_f : (float)RawInt(b);
  float n;

  switch (op) {
  case OP_ADD:
    n = a_f + b_f;
    break;
  case OP_SUB:
    n = a_f - b_f;
    break;
  case OP_MUL:
    n = a_f * b_f;
    break;
  case OP_DIV:
    if (b_f == 0.0) {
      RuntimeError(vm, "Divide by zero");
      return;
    }
    n = a_f / b_f;
    break;
  case OP_EXP:
    n = powf(a_f, b_f);
    break;
  default:
    RuntimeError(vm, "Arithmetic error: Unknown op %s", OpStr(op));
    return;
  }

  VecPush(vm->stack, NumVal(n));
}

void NotOp(struct VM *vm)
{
  Val val = VecPop(vm->stack);

  if (IsFalsey(val)) {
    VecPush(vm->stack, SymbolFor("true"));
  } else {
    VecPush(vm->stack, SymbolFor("false"));
  }
}

void ConsOp(struct VM *vm)
{
  Val tail = VecPop(vm->stack);
  Val head = VecPop(vm->stack);
  Val pair = MakePair(head, tail);
  VecPush(vm->stack, pair);
}
