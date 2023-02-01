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
  [OP_HALT] =     { "halt",   ARGS_NONE   },
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
  [OP_LOOKUP] =   { "lookup", ARGS_NONE   },
  [OP_PAIR] =     { "pair",   ARGS_NONE   },
  [OP_LIST] =     { "list",   ARGS_INT    },
  [OP_APPLY] =    { "apply",  ARGS_NONE   },
  [OP_JUMP] =     { "jump",   ARGS_INT    },
  [OP_LAMBDA] =   { "lambda", ARGS_NONE   },
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
  case ARGS_INT:      return 2;
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

void CompareOp(struct VM *vm, OpCode op)
{
  Val b = VecPop(vm->stack);
  Val a = VecPop(vm->stack);

  if (!IsNumeric(a) || !IsNumeric(b)) {
    RuntimeError(vm, "Comparison error");
    return;
  }

  bool result;
  switch (op) {
  case OP_EQUAL:  result = RawNum(a) == RawNum(b); break;
  case OP_LT:     result = RawNum(a) < RawNum(b); break;
  case OP_GT:     result = RawNum(a) > RawNum(b); break;
  default:
    RuntimeError(vm, "Unknown op");
    return;
  }

  VecPush(vm->stack, result ? SymbolFor("true") : SymbolFor("false"));
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

void PairOp(struct VM *vm)
{
  Val tail = VecPop(vm->stack);
  Val head = VecPop(vm->stack);
  Val pair = MakePair(&vm->heap, head, tail);
  VecPush(vm->stack, pair);
}

void ListOp(struct VM *vm, u32 num)
{
  Val list = nil;
  for (u32 i = 0; i < num; i++) {
    Val item = VecPop(vm->stack);
    list = MakePair(&vm->heap, item, list);
  }
  VecPush(vm->stack, list);
}

void LambdaOp(struct VM *vm)
{
  Val code = VecPop(vm->stack);
  Val params = VecPop(vm->stack);

  Val proc = nil;
  proc = MakePair(&vm->heap, vm->env, proc);
  proc = MakePair(&vm->heap, params, proc);
  proc = MakePair(&vm->heap, code, proc);

  VecPush(vm->stack, proc);
}
