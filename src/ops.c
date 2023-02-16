#include "ops.h"
#include "vec.h"
#include "value.h"
#include "vm.h"
#include "mem.h"
#include "printer.h"
#include "env.h"
#include "vm.h"
#include "native.h"

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
static void MapOp(VM *vm, OpCode op);
static void LambdaOp(VM *vm, OpCode op);
static void NegOp(VM *vm, OpCode op);
static void ArithmeticOp(VM *vm, OpCode op);
static void NotOp(VM *vm, OpCode op);
static void CompareOp(VM *vm, OpCode op);
static void DefineOp(VM *vm, OpCode op);
static void LookupOp(VM *vm, OpCode op);
static void AccessOp(VM *vm, OpCode op);
static void ModuleOp(VM *vm, OpCode op);
static void ImportOp(VM *vm, OpCode op);
static void UseOp(VM *vm, OpCode op);
static void PushScopeOp(VM *vm, OpCode op);
static void PopScopeOp(VM *vm, OpCode op);
static void BranchOp(VM *vm, OpCode op);
static void JumpOp(VM *vm, OpCode op);
static void CallOp(VM *vm, OpCode op);
static void ReturnOp(VM *vm, OpCode op);
static void ApplyOp(VM *vm, OpCode op);

static OpInfo ops[] = {
  [OP_HALT] =     { "halt",     ARGS_NONE,  &StatusOp     },
  [OP_BREAK] =    { "break",    ARGS_NONE,  &StatusOp     },
  [OP_POP] =      { "pop",      ARGS_NONE,  &PopOp        },
  [OP_DUP] =      { "dup",      ARGS_NONE,  &DupOp        },
  [OP_CONST] =    { "const",    ARGS_VAL,   &ConstOp      },
  [OP_TRUE] =     { "true",     ARGS_NONE,  &ConstOp      },
  [OP_FALSE] =    { "false",    ARGS_NONE,  &ConstOp      },
  [OP_NIL] =      { "nil",      ARGS_NONE,  &ConstOp      },
  [OP_ZERO] =     { "zero",     ARGS_NONE,  &ConstOp      },
  [OP_PAIR] =     { "pair",     ARGS_NONE,  &PairOp       },
  [OP_LIST] =     { "list",     ARGS_INT,   &ListOp       },
  [OP_DICT] =     { "dict",     ARGS_INT,   &MapOp        },
  [OP_LAMBDA] =   { "lambda",   ARGS_INT,   &LambdaOp     },
  [OP_NEG] =      { "neg",      ARGS_NONE,  &NegOp        },
  [OP_ADD] =      { "add",      ARGS_NONE,  &ArithmeticOp },
  [OP_SUB] =      { "sub",      ARGS_NONE,  &ArithmeticOp },
  [OP_MUL] =      { "mul",      ARGS_NONE,  &ArithmeticOp },
  [OP_DIV] =      { "div",      ARGS_NONE,  &ArithmeticOp },
  [OP_EXP] =      { "exp",      ARGS_NONE,  &ArithmeticOp },
  [OP_NOT] =      { "not",      ARGS_NONE,  &NotOp        },
  [OP_EQUAL] =    { "equal",    ARGS_NONE,  &CompareOp    },
  [OP_GT] =       { "gt",       ARGS_NONE,  &CompareOp    },
  [OP_LT] =       { "lt",       ARGS_NONE,  &CompareOp    },
  [OP_DEFINE] =   { "define",   ARGS_NONE,  &DefineOp     },
  [OP_LOOKUP] =   { "lookup",   ARGS_NONE,  &LookupOp     },
  [OP_ACCESS] =   { "access",   ARGS_NONE,  &AccessOp     },
  [OP_MODULE] =   { "module",   ARGS_NONE,  &ModuleOp     },
  [OP_IMPORT] =   { "import",   ARGS_NONE,  &ImportOp     },
  [OP_USE] =      { "use",      ARGS_NONE,  &UseOp        },
  [OP_SCOPE] =    { "scope",    ARGS_NONE,  &PushScopeOp  },
  [OP_UNSCOPE] =  { "unscope",  ARGS_NONE,  &PopScopeOp   },
  [OP_BRANCH] =   { "branch",   ARGS_INT,   &BranchOp     },
  [OP_JUMP] =     { "jump",     ARGS_INT,   &JumpOp       },
  [OP_CALL] =     { "call",     ARGS_INT,   &CallOp       },
  [OP_RETURN] =   { "return",   ARGS_NONE,  &ReturnOp     },
  [OP_APPLY] =    { "apply",    ARGS_INT,   &ApplyOp      },
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
  PrintVMVal(vm, val);
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
    } else if (op == OP_MUL) {
      StackPush(vm, IntVal(RawInt(a) * RawInt(b)));
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

static void MapOp(VM *vm, OpCode op)
{
  u32 num = ReadByte(vm);

  Val map = MakeMap(&vm->heap, num);

  for (u32 i = 0; i < num; i++) {
    Val val = StackPop(vm);
    Val key = StackPop(vm);
    MapPut(vm->heap, map, key, val);
  }

  StackPush(vm, map);
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
    PrintEnv(vm);
  }
}

static void AccessOp(VM *vm, OpCode op)
{
  Val var = StackPop(vm);
  Val dict = StackPop(vm);

  if (!IsMap(dict)) {
    RuntimeError(vm, "Can't access this");
  } else if (!MapHasKey(vm->heap, dict, var)) {
    StackPush(vm, nil);
  } else {
    Val val = MapGet(vm->heap, dict, var);
    StackPush(vm, val);
  }
}

static void PushScopeOp(VM *vm, OpCode op)
{
  vm->env = ExtendEnv(vm, vm->env);
}

static void PopScopeOp(VM *vm, OpCode op)
{
  vm->env = ParentEnv(vm, vm->env);
}

static void ModuleOp(VM *vm, OpCode op)
{
  Val name = StackPop(vm);
  Val mod = FrameMap(vm, vm->env);
  PutModule(vm, name, mod);
  vm->env = ParentEnv(vm, vm->env);
}

static void ImportOp(VM *vm, OpCode op)
{
  Val name = StackPop(vm);
  Val mod = GetModule(vm, name);
  if (IsNil(mod)) {
    RuntimeError(vm, "Module not defined: %s\n", SymbolName(&vm->chunk->strings, name));
    return;
  }

  for (u32 i = 0; i < MapSize(vm->heap, mod); i++) {
    Val key = MapKeyAt(vm->heap, mod, i);
    if (!IsNil(key)) {
      Val val = MapValAt(vm->heap, mod, i);
      Define(vm, key, val, vm->env);
    }
  }
}

static void UseOp(VM *vm, OpCode op)
{
  Val name = StackPop(vm);
  Val mod = GetModule(vm, name);
  if (IsNil(mod)) {
    RuntimeError(vm, "Module not defined: %s\n", SymbolName(&vm->chunk->strings, name));
    return;
  }

  Define(vm, name, mod, vm->env);
  StackPush(vm, SymbolFor("ok"));
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

static void Bind(VM *vm, Val params, Val env, u32 num_args)
{
  while (!IsNil(params)) {
    if (num_args == 0) {
      // error: too few args
    }
    num_args--;

    Val var = Head(vm->heap, params);
    Val val = StackPop(vm);
    Define(vm, var, val, env);

    params = Tail(vm->heap, params);
  }

  for (u32 i = 0; i < num_args; i++) {
    // too many args
    StackPop(vm);
  }
}

static void ApplyOp(VM *vm, OpCode op)
{
  u8 num_args = ReadByte(vm);
  Val proc = StackPeek(vm, num_args);

  if (!IsPair(proc)) return;

  if (Eq(Head(vm->heap, proc), SymbolFor("native"))) {
    DoNative(vm, Tail(vm->heap, proc), num_args);
    StackPop(vm); // pop the proc
    return;
  }

  Val code = First(vm->heap, proc);
  Val params = Second(vm->heap, proc);
  Val env = ExtendEnv(vm, Third(vm->heap, proc));
  Bind(vm, params, env, num_args);
  StackPop(vm); // pop the proc

  vm->pc = RawInt(code);
  vm->env = env;
}

static void CallOp(VM *vm, OpCode op)
{
  u8 num_args = ReadByte(vm);
  Val proc = StackPeek(vm, num_args);

  if (!IsPair(proc)) return;

  if (Eq(Head(vm->heap, proc), SymbolFor("native"))) {
    DoNative(vm, Tail(vm->heap, proc), num_args);
    StackPop(vm); // pop the proc
    return;
  }

  Val code = First(vm->heap, proc);
  Val params = Second(vm->heap, proc);
  Val env = ExtendEnv(vm, Third(vm->heap, proc));

  Bind(vm, params, env, num_args);
  StackPop(vm); // pop the proc

  StackPush(vm, IntVal(vm->pc));
  StackPush(vm, vm->env);
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
