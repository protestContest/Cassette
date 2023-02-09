#include "ops.h"
#include "vec.h"
#include "value.h"
#include "vm.h"
#include "mem.h"
#include "printer.h"
#include "env.h"
#include "vm.h"

typedef void (*OpFn)(VM *vm, OpCode op);

typedef struct {
  char *name;
  ArgFormat format;
  OpFn impl;
} OpInfo;

static void StatusOp(VM *vm, OpCode op);
static void PrintOp(VM *vm, OpCode op);
static void PopOp(VM *vm, OpCode op);
static void DupOp(VM *vm, OpCode op);
static void ConstOp(VM *vm, OpCode op);
static void PairOp(VM *vm, OpCode op);
static void ListOp(VM *vm, OpCode op);
static void DictOp(VM *vm, OpCode op);
static void LambdaOp(VM *vm, OpCode op);
static void NegOp(VM *vm, OpCode op);
static void ArithmeticOp(VM *vm, OpCode op);
static void NotOp(VM *vm, OpCode op);
static void CompareOp(VM *vm, OpCode op);
static void DefineOp(VM *vm, OpCode op);
static void LookupOp(VM *vm, OpCode op);
static void AccessOp(VM *vm, OpCode op);
static void BranchOp(VM *vm, OpCode op);
static void JumpOp(VM *vm, OpCode op);
static void CallOp(VM *vm, OpCode op);
static void ReturnOp(VM *vm, OpCode op);
static void ApplyOp(VM *vm, OpCode op);

static OpInfo ops[] = {
  [OP_HALT] =     { "halt",   ARGS_NONE,  &StatusOp     },
  [OP_BREAK] =    { "break",  ARGS_NONE,  &StatusOp     },
  [OP_POP] =      { "pop",    ARGS_NONE,  &PopOp        },
  [OP_DUP] =      { "dup",    ARGS_NONE,  &DupOp        },
  [OP_CONST] =    { "const",  ARGS_VAL,   &ConstOp      },
  [OP_TRUE] =     { "true",   ARGS_NONE,  &ConstOp      },
  [OP_FALSE] =    { "false",  ARGS_NONE,  &ConstOp      },
  [OP_NIL] =      { "nil",    ARGS_NONE,  &ConstOp      },
  [OP_ZERO] =     { "zero",   ARGS_NONE,  &ConstOp      },
  [OP_PAIR] =     { "pair",   ARGS_NONE,  &PairOp       },
  [OP_LIST] =     { "list",   ARGS_INT,   &ListOp       },
  [OP_DICT] =     { "dict",   ARGS_INT,   &DictOp       },
  [OP_LAMBDA] =   { "lambda", ARGS_INT,   &LambdaOp     },
  [OP_NEG] =      { "neg",    ARGS_NONE,  &NegOp        },
  [OP_ADD] =      { "add",    ARGS_NONE,  &ArithmeticOp },
  [OP_SUB] =      { "sub",    ARGS_NONE,  &ArithmeticOp },
  [OP_MUL] =      { "mul",    ARGS_NONE,  &ArithmeticOp },
  [OP_DIV] =      { "div",    ARGS_NONE,  &ArithmeticOp },
  [OP_EXP] =      { "exp",    ARGS_NONE,  &ArithmeticOp },
  [OP_NOT] =      { "not",    ARGS_NONE,  &NotOp        },
  [OP_EQUAL] =    { "equal",  ARGS_NONE,  &CompareOp    },
  [OP_GT] =       { "gt",     ARGS_NONE,  &CompareOp    },
  [OP_LT] =       { "lt",     ARGS_NONE,  &CompareOp    },
  [OP_DEFINE] =   { "define", ARGS_NONE,  &DefineOp     },
  [OP_LOOKUP] =   { "lookup", ARGS_NONE,  &LookupOp     },
  [OP_ACCESS] =   { "access", ARGS_NONE,  &AccessOp     },
  [OP_BRANCH] =   { "branch", ARGS_INT,   &BranchOp     },
  [OP_JUMP] =     { "jump",   ARGS_INT,   &JumpOp       },
  [OP_CALL] =     { "call",   ARGS_NONE,  &CallOp       },
  [OP_RETURN] =   { "return", ARGS_NONE,  &ReturnOp     },
  [OP_APPLY] =    { "apply",  ARGS_NONE,  &ApplyOp      },
};

void DoOp(VM *vm, OpCode op)
{
  OpFn fn = ops[op].impl;
  fn(vm, op);
}

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
  return IsNil(value) || Eq(value, SymbolFor("false"));
}

static void StatusOp(VM *vm, OpCode op)
{
  switch (op) {
  case OP_HALT:
    vm->status = VM_Halted;
    break;
  case OP_BREAK:
    vm->status = VM_Debug;
    break;
  default:
    break;
  }
}

static void PrintOp(VM *vm, OpCode op)
{
  Val val = StackPop(vm);
  PrintVal(vm->heap, vm->chunk->symbols, val);
}

static void PopOp(VM *vm, OpCode op)
{
  StackPop(vm);
}

static void DupOp(VM *vm, OpCode op)
{
  Val val = StackPeek(vm, 0);
  StackPush(vm, val);
}

static void ConstOp(VM *vm, OpCode op)
{
  switch (op) {
  case OP_TRUE:
    StackPush(vm, SymbolFor("true"));
    break;
  case OP_FALSE:
    StackPush(vm, SymbolFor("false"));
    break;
  case OP_NIL:
    StackPush(vm, nil);
    break;
  case OP_ZERO:
    StackPush(vm, IntVal(0));
    break;
  case OP_CONST:
    StackPush(vm, ReadConst(vm));
    break;
  default:
    break;
  }
}

static void NegOp(VM *vm, OpCode op)
{
  Val val = StackPop(vm);
  if (IsNum(val)) {
    StackPush(vm, NumVal(-val.as_f));
  } else if (IsInt(val)) {
    StackPush(vm, IntVal(-RawInt(val)));
  } else {
    RuntimeError(vm, "Arithmetic error");
  }
}

static void ArithmeticOp(VM *vm, OpCode op)
{
  Val b = StackPop(vm);
  Val a = StackPop(vm);

  if (!IsNumeric(a) || !IsNumeric(b)) {
    RuntimeError(vm, "Arithmetic error");
    return;
  }

  if (IsInt(a) && IsInt(b)) {
    if (op == OP_ADD) {
      StackPush(vm, IntVal(RawInt(a) + RawInt(b)));
      return;
    } else if (op == OP_SUB) {
      StackPush(vm, IntVal(RawInt(a) - RawInt(b)));
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

  StackPush(vm, NumVal(n));
}

static void CompareOp(VM *vm, OpCode op)
{
  Val b = StackPop(vm);
  Val a = StackPop(vm);

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

  StackPush(vm, result ? SymbolFor("true") : SymbolFor("false"));
}

static void NotOp(VM *vm, OpCode op)
{
  Val val = StackPop(vm);

  if (IsFalsey(val)) {
    StackPush(vm, SymbolFor("true"));
  } else {
    StackPush(vm, SymbolFor("false"));
  }
}

static void PairOp(VM *vm, OpCode op)
{
  Val tail = StackPop(vm);
  Val head = StackPop(vm);
  Val pair = MakePair(&vm->heap, head, tail);
  StackPush(vm, pair);
}

static void ListOp(VM *vm, OpCode op)
{
  u32 num = ReadByte(vm);

  Val list = nil;
  for (u32 i = 0; i < num; i++) {
    Val item = StackPop(vm);
    list = MakePair(&vm->heap, item, list);
  }
  StackPush(vm, list);
}

static void DictOp(VM *vm, OpCode op)
{
  u32 num = ReadByte(vm);

  Val dict = MakeDict(&vm->heap, num);

  for (u32 i = 0; i < num; i++) {
    Val val = StackPop(vm);
    Val key = StackPop(vm);
    DictPut(vm->heap, dict, key, val);
  }

  StackPush(vm, dict);
}

static void LambdaOp(VM *vm, OpCode op)
{
  u32 num_params = ReadByte(vm);
  Val code = StackPop(vm);

  Val params = nil;
  for (u32 i = 0; i < num_params; i++) {
    Val param = StackPeek(vm, num_params - 1 - i);
    params = MakePair(&vm->heap, param, params);
  }
  StackTrunc(vm, num_params);

  Val proc = nil;
  proc = MakePair(&vm->heap, vm->env, proc);
  proc = MakePair(&vm->heap, params, proc);
  proc = MakePair(&vm->heap, code, proc);

  StackPush(vm, proc);
}

static void DefineOp(VM *vm, OpCode op)
{
  Val val = StackPop(vm);
  Val var = StackPop(vm);

  Define(vm, var, val, vm->env);
  StackPush(vm, SymbolFor("ok"));
}

static void LookupOp(VM *vm, OpCode op)
{
  Val var = StackPop(vm);

  Result result = Lookup(vm, var, vm->env);
  if (result.status == Ok) {
    StackPush(vm, result.value);
  } else {
    RuntimeError(vm, "Undefined symbol");
  }
}

static void AccessOp(VM *vm, OpCode op)
{
  Val var = StackPop(vm);
  Val dict = StackPop(vm);

  if (!IsDict(dict)) {
    RuntimeError(vm, "Can't access this");
  } else if (!DictHasKey(vm->heap, dict, var)) {
    RuntimeError(vm, "Invalid access");
  } else {
    Val val = DictGet(vm->heap, dict, var);
    StackPush(vm, val);
  }
}

static void BranchOp(VM *vm, OpCode op)
{
  Val test = StackPop(vm);
  i8 branch = (i8)ReadByte(vm);
  if (!IsFalsey(test)) {
    vm->pc += branch;
  }
}

static void JumpOp(VM *vm, OpCode op)
{
  vm->pc += (i8)ReadByte(vm);
}

static void ApplyOp(VM *vm, OpCode op)
{
  Val proc = StackPop(vm);
  Val code = First(vm->heap, proc);
  Val params = Second(vm->heap, proc);
  Val env = Third(vm->heap, proc);

  env = ExtendEnv(vm, env);
  while (!IsNil(params)) {
    Val var = Head(vm->heap, params);
    Val val = StackPop(vm);
    Define(vm, var, val, env);
    params = Tail(vm->heap, params);
  }

  vm->pc = RawInt(code);
  vm->env = env;
}

static void ReturnOp(VM *vm, OpCode op)
{
  Val result = StackPop(vm);
  Val env = StackPop(vm);
  Val cont = StackPop(vm);
  StackPush(vm, result);
  vm->env = env;
  vm->pc = RawInt(cont);
}

static void CallOp(VM *vm, OpCode op)
{
  if (!IsPair(StackPeek(vm, 0))) return;

  Val proc = StackPop(vm);
  Val code = First(vm->heap, proc);
  Val params = Second(vm->heap, proc);
  Val env = Third(vm->heap, proc);

  env = ExtendEnv(vm, env);
  while (!IsNil(params)) {
    Val var = Head(vm->heap, params);
    Val val = StackPop(vm);
    Define(vm, var, val, env);

    params = Tail(vm->heap, params);
  }

  StackPush(vm, IntVal(vm->pc));
  StackPush(vm, vm->env);
  vm->pc = RawInt(code);
  vm->env = env;
}
