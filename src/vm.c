#include "vm.h"
#include "mem.h"
#include "env.h"
#include "parse.h"
#include <univ.h>

typedef VMStatus (*OpFn)(VM *vm);

static VMStatus OpNoop(VM *vm);
static VMStatus OpConst(VM *vm);
static VMStatus OpAdd(VM *vm);
static VMStatus OpSub(VM *vm);
static VMStatus OpMul(VM *vm);
static VMStatus OpDiv(VM *vm);
static VMStatus OpRem(VM *vm);
static VMStatus OpAnd(VM *vm);
static VMStatus OpOr(VM *vm);
static VMStatus OpComp(VM *vm);
static VMStatus OpLt(VM *vm);
static VMStatus OpGt(VM *vm);
static VMStatus OpEq(VM *vm);
static VMStatus OpNeg(VM *vm);
static VMStatus OpNot(VM *vm);
static VMStatus OpShift(VM *vm);
static VMStatus OpNil(VM *vm);
static VMStatus OpPair(VM *vm);
static VMStatus OpHead(VM *vm);
static VMStatus OpTail(VM *vm);
static VMStatus OpTuple(VM *vm);
static VMStatus OpLen(VM *vm);
static VMStatus OpGet(VM *vm);
static VMStatus OpSet(VM *vm);
static VMStatus OpStr(VM *vm);
static VMStatus OpBin(VM *vm);
static VMStatus OpJoin(VM *vm);
static VMStatus OpTrunc(VM *vm);
static VMStatus OpSkip(VM *vm);
static VMStatus OpJmp(VM *vm);
static VMStatus OpBr(VM *vm);
static VMStatus OpTrap(VM *vm);
static VMStatus OpPos(VM *vm);
static VMStatus OpGoto(VM *vm);
static VMStatus OpHalt(VM *vm);
static VMStatus OpDup(VM *vm);
static VMStatus OpDrop(VM *vm);
static VMStatus OpSwap(VM *vm);
static VMStatus OpOver(VM *vm);
static VMStatus OpRot(VM *vm);
static VMStatus OpGetEnv(VM *vm);
static VMStatus OpSetEnv(VM *vm);
static VMStatus OpLookup(VM *vm);
static VMStatus OpDefine(VM *vm);

static OpFn ops[] = {
  [opNoop]    = OpNoop,
  [opConst]   = OpConst,
  [opAdd]     = OpAdd,
  [opSub]     = OpSub,
  [opMul]     = OpMul,
  [opDiv]     = OpDiv,
  [opRem]     = OpRem,
  [opAnd]     = OpAnd,
  [opOr]      = OpOr,
  [opComp]    = OpComp,
  [opLt]      = OpLt,
  [opGt]      = OpGt,
  [opEq]      = OpEq,
  [opNeg]     = OpNeg,
  [opNot]     = OpNot,
  [opShift]   = OpShift,
  [opNil]     = OpNil,
  [opPair]    = OpPair,
  [opHead]    = OpHead,
  [opTail]    = OpTail,
  [opTuple]   = OpTuple,
  [opLen]     = OpLen,
  [opGet]     = OpGet,
  [opSet]     = OpSet,
  [opStr]     = OpStr,
  [opBin]     = OpBin,
  [opJoin]    = OpJoin,
  [opTrunc]   = OpTrunc,
  [opSkip]    = OpSkip,
  [opJmp]     = OpJmp,
  [opBr]      = OpBr,
  [opTrap]    = OpTrap,
  [opPos]     = OpPos,
  [opGoto]    = OpGoto,
  [opHalt]    = OpHalt,
  [opDup]     = OpDup,
  [opDrop]    = OpDrop,
  [opSwap]    = OpSwap,
  [opOver]    = OpOver,
  [opRot]     = OpRot,
  [opGetEnv]  = OpGetEnv,
  [opSetEnv]  = OpSetEnv,
  [opLookup]  = OpLookup,
  [opDefine]  = OpDefine,
};

void InitVM(VM *vm, Module *mod)
{
  vm->mod = mod;
  vm->status = ok;
  vm->pc = 0;
  vm->env = 0;
  vm->primitives = 0;
  FreeVec(vm->stack);
  ImportSymbols(mod->data, VecCount(mod->data));
  InitHashMap(&vm->primMap);
}

void DefinePrimitive(val id, PrimFn fn, VM *vm)
{
  HashMapSet(&vm->primMap, id, VecCount(vm->primitives));
  VecPush(vm->primitives, fn);
}

VMStatus VMStep(VM *vm)
{
  return ops[vm->mod->code[vm->pc]](vm);
}

void PrintSourceFrom(u32 index, char *src)
{
  char *start = src+index;
  char *end = LineEnd(start);
  printf("%*.*s", (i32)(end-start), (i32)(end-start), start);
}

void VMRun(VM *vm, char *src)
{
  u32 pc;
  while (vm->status == ok && vm->pc < VecCount(vm->mod->code)) {
    pc = vm->pc;
    TraceInst(vm);
    vm->status = ops[vm->mod->code[vm->pc++]](vm);
    PrintStack(vm);
    PrintSourceFrom(GetSourcePos(pc, vm->mod), src);
    printf("\n");
  }
}

val StackPop(VM *vm)
{
  return VecPop(vm->stack);
}

void StackPush(val value, VM *vm)
{
  VecPush(vm->stack, value);
}

#define OneArg() \
  i32 a;\
  CheckStack(vm, 1);\
  a = StackPop(vm)

#define TwoArgs() \
  i32 a, b;\
  CheckStack(vm, 2);\
  b = StackPop(vm);\
  a = StackPop(vm)

#define ThreeArgs() \
  i32 a, b, c;\
  CheckStack(vm, 3);\
  c = StackPop(vm);\
  b = StackPop(vm);\
  a = StackPop(vm)

#define UnaryOp(op)     StackPush(IntVal(op RawVal(a)), vm)
#define BinOp(op)       StackPush(IntVal(RawVal(a) op RawVal(b)), vm)
#define CheckBounds(n)  if ((n) < 0 || (n) > VecCount(vm->mod->code)) return outOfBounds

static VMStatus OpNoop(VM *vm)
{
  return ok;
}

static VMStatus OpConst(VM *vm)
{
  i32 num = ReadInt(&vm->pc, vm->mod);
  StackPush(vm->mod->constants[num], vm);
  return ok;
}

static VMStatus OpAdd(VM *vm)
{
  TwoArgs();
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(+);
  return ok;
}

static VMStatus OpSub(VM *vm)
{
  TwoArgs();
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(-);
  return ok;
}

static VMStatus OpMul(VM *vm)
{
  TwoArgs();
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(*);
  return ok;
}

static VMStatus OpDiv(VM *vm)
{
  TwoArgs();
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  if (RawVal(b) == 0) return divideByZero;
  BinOp(/);
  return ok;
}

static VMStatus OpRem(VM *vm)
{
  TwoArgs();
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  if (RawVal(b) == 0) return divideByZero;
  BinOp(%);
  return ok;
}

static VMStatus OpAnd(VM *vm)
{
  TwoArgs();
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(&);
  return ok;
}

static VMStatus OpOr(VM *vm)
{
  TwoArgs();
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(|);
  return ok;
}

static VMStatus OpComp(VM *vm)
{
  OneArg();
  if (!IsInt(a)) return invalidType;
  UnaryOp(~);
  return ok;
}

static VMStatus OpLt(VM *vm)
{
  TwoArgs();
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(<);
  return ok;
}

static VMStatus OpGt(VM *vm)
{
  TwoArgs();
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(>);
  return ok;
}

static VMStatus OpEq(VM *vm)
{
  TwoArgs();
  StackPush(IntVal(a == b), vm);
  return ok;
}

static VMStatus OpNeg(VM *vm)
{
  OneArg();
  if (!IsInt(a)) return invalidType;
  UnaryOp(-);
  return ok;
}

static VMStatus OpNot(VM *vm)
{
  OneArg();
  StackPush(IntVal(RawVal(a) != 0), vm);
  return ok;
}

static VMStatus OpShift(VM *vm)
{
  TwoArgs();
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(<<);
  return ok;
}

static VMStatus OpNil(VM *vm)
{
  StackPush(0, vm);
  return ok;
}

static VMStatus OpPair(VM *vm)
{
  TwoArgs();
  StackPush(Pair(b, a), vm);
  return ok;
}

static VMStatus OpHead(VM *vm)
{
  OneArg();
  if (!IsPair(a)) return invalidType;
  StackPush(Head(a), vm);
  return ok;
}

static VMStatus OpTail(VM *vm)
{
  OneArg();
  if (!IsPair(a)) return invalidType;
  StackPush(Tail(a), vm);
  return ok;
}

static VMStatus OpTuple(VM *vm)
{
  i32 count = ReadInt(&vm->pc, vm->mod);
  StackPush(Tuple(count), vm);
  return ok;
}

static VMStatus OpLen(VM *vm)
{
  OneArg();
  if (IsPair(a)) {
    StackPush(IntVal(ListLength(a)), vm);
  } else if (IsTuple(a)) {
    StackPush(IntVal(TupleLength(a)), vm);
  } else if (IsBinary(a)) {
    StackPush(IntVal(BinaryLength(a)), vm);
  } else {
    return invalidType;
  }
  return ok;
}

static VMStatus OpGet(VM *vm)
{
  TwoArgs();
  if (!IsInt(b)) return invalidType;
  if (IsPair(a)) {
    if (RawVal(b) < 0 || RawVal(b) >= ListLength(a)) return outOfBounds;
    StackPush(ListGet(a, RawVal(b)), vm);
  } else if (IsTuple(a)) {
    if (RawVal(b) < 0 || RawVal(b) >= TupleLength(a)) return outOfBounds;
    StackPush(TupleGet(a, RawVal(b)), vm);
  } else if (IsBinary(a)) {
    if (RawVal(b) < 0 || RawVal(b) >= BinaryLength(a)) return outOfBounds;
    StackPush(BinaryGet(a, RawVal(b)), vm);
  } else {
    return invalidType;
  }
  return ok;
}

static VMStatus OpSet(VM *vm)
{
  ThreeArgs();
  if (!IsInt(b)) return invalidType;
  if (IsTuple(a)) {
    TupleSet(a, RawVal(b), c);
  } else if (IsBinary(a)) {
    BinarySet(a, RawVal(b), c);
  } else {
    return invalidType;
  }
  return ok;
}

static VMStatus OpStr(VM *vm)
{
  char *name;
  OneArg();
  if (!IsSym(a)) return invalidType;
  name = SymbolName(RawVal(a));
  StackPush(BinaryFrom(name, strlen(name)), vm);
  return ok;
}

static VMStatus OpBin(VM *vm)
{
  i32 count = ReadInt(&vm->pc, vm->mod);
  StackPush(Binary(count), vm);
  return ok;
}

static VMStatus OpJoin(VM *vm)
{
  TwoArgs();
  if (ValType(a) != ValType(b)) return invalidType;
  if (IsPair(a)) {
    StackPush(ListJoin(a, b), vm);
  } else if (IsTuple(a)) {
    StackPush(TupleJoin(a, b), vm);
  } else if (IsBinary(a)) {
    StackPush(BinaryJoin(a, b), vm);
  } else {
    return invalidType;
  }
  return ok;
}

static VMStatus OpTrunc(VM *vm)
{
  TwoArgs();
  if (!IsInt(b)) return invalidType;
  if (IsPair(a)) {
    StackPush(ListTrunc(a, RawVal(b)), vm);
  } else if (IsTuple(a)) {
    StackPush(TupleTrunc(a, RawVal(b)), vm);
  } else if (IsBinary(a)) {
    StackPush(BinaryTrunc(a, RawVal(b)), vm);
  } else {
    return invalidType;
  }
  return ok;
}

static VMStatus OpSkip(VM *vm)
{
  TwoArgs();
  if (!IsInt(b)) return invalidType;
  if (IsPair(a)) {
    StackPush(ListSkip(a, RawVal(b)), vm);
  } else if (IsTuple(a)) {
    StackPush(TupleSkip(a, RawVal(b)), vm);
  } else if (IsBinary(a)) {
    StackPush(BinarySkip(a, RawVal(b)), vm);
  } else {
    return invalidType;
  }
  return ok;
}

static VMStatus OpJmp(VM *vm)
{
  i32 n = ReadInt(&vm->pc, vm->mod);
  CheckBounds(vm->pc + n);
  vm->pc += n;
  return ok;
}

static VMStatus OpBr(VM *vm)
{
  i32 n = ReadInt(&vm->pc, vm->mod);
  OneArg();
  if (RawVal(a)) {
    CheckBounds(vm->pc + n);
    vm->pc += n;
  }
  return ok;
}

static VMStatus OpTrap(VM *vm)
{
  u32 id = ReadInt(&vm->pc, vm->mod);
  if (HashMapContains(&vm->primMap, id)) {
    return vm->primitives[HashMapGet(&vm->primMap, id)](vm);
  }
  return unhandledTrap;
}

static VMStatus OpPos(VM *vm)
{
  i32 n = ReadInt(&vm->pc, vm->mod);
  StackPush(IntVal(vm->pc + n), vm);
  return ok;
}

static VMStatus OpGoto(VM *vm)
{
  OneArg();
  if (!IsInt(a)) return invalidType;
  CheckBounds(RawVal(a));
  vm->pc = RawVal(a);
  return ok;
}

static VMStatus OpHalt(VM *vm)
{
  vm->pc = VecCount(vm->mod->code);
  return ok;
}

static VMStatus OpDup(VM *vm)
{
  OneArg();
  StackPush(a, vm);
  StackPush(a, vm);
  return ok;
}

static VMStatus OpDrop(VM *vm)
{
  if (VecCount(vm->stack) < 1) return stackUnderflow;
  VecPop(vm->stack);
  return ok;
}

static VMStatus OpSwap(VM *vm)
{
  TwoArgs();
  StackPush(b, vm);
  StackPush(a, vm);
  return ok;
}

static VMStatus OpOver(VM *vm)
{
  TwoArgs();
  StackPush(a, vm);
  StackPush(b, vm);
  StackPush(a, vm);
  return ok;
}

static VMStatus OpRot(VM *vm)
{
  ThreeArgs();
  StackPush(b, vm);
  StackPush(c, vm);
  StackPush(a, vm);
  return ok;
}

static VMStatus OpGetEnv(VM *vm)
{
  StackPush(vm->env, vm);
  return ok;
}

static VMStatus OpSetEnv(VM *vm)
{
  if (VecCount(vm->stack) < 1) return stackUnderflow;
  vm->env = StackPop(vm);
  return ok;
}

static VMStatus OpLookup(VM *vm)
{
  i32 n = ReadInt(&vm->pc, vm->mod);
  StackPush(Lookup(n, vm->env), vm);
  return ok;
}

static VMStatus OpDefine(VM *vm)
{
  i32 n = ReadInt(&vm->pc, vm->mod);
  OneArg();
  if (!Define(a, n, vm->env)) return outOfBounds;
  return ok;
}

void TraceInst(VM *vm)
{
  u32 index = vm->pc;
  char *op = DisassembleOp(&index, 0, vm->mod);
  u32 i, len = strlen(op);
  printf("%s ", op);
  for (i = 0; i < 20 - len; i++) printf(" ");
}

u32 PrintStack(VM *vm)
{
  u32 i, printed = 0;
  char *str;
  for (i = 0; i < 3; i++) {
    if (i >= VecCount(vm->stack)) break;
    str = ValStr(vm->stack[VecCount(vm->stack) - i - 1]);
    printed += printf("%s ", str);
    free(str);
  }
  if (VecCount(vm->stack) > 3) printed += printf("...");
  for (i = 0; (i32)i < 30 - (i32)printed; i++) printf(" ");
  return printed;
}

val VMError(VM *vm)
{
  u32 srcPos = GetSourcePos(vm->pc-1, vm->mod);

  switch (vm->status) {
  case stackUnderflow:
    return MakeError("Stack Underflow", srcPos);
  case invalidType:
    return MakeError("Invalid Type", srcPos);
  case divideByZero:
    return MakeError("Divdie by Zero", srcPos);
  case outOfBounds:
    return MakeError("Out of Bounds", srcPos);
  case unhandledTrap:
    return MakeError("Trap", srcPos);
  default:
    return nil;
  }
}

