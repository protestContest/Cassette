#include "vm.h"
#include "debug.h"
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
  vm->status = vmOk;
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
  debug("%*.*s", (i32)(end-start), (i32)(end-start), start);
}

void VMRun(VM *vm, char *src)
{
  while (vm->status == vmOk && vm->pc < VecCount(vm->mod->code)) {
    TraceInst(vm);
    PrintStack(vm);
    PrintSourceFrom(GetSourcePos(vm->pc, vm->mod), src);
    debug("\n");
    vm->status = ops[vm->mod->code[vm->pc++]](vm);
  }
  if (vm->status == vmOk) {
    TraceInst(vm);
    PrintStack(vm);
    PrintSourceFrom(GetSourcePos(vm->pc, vm->mod), src);
    debug("\n");
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

void RunGC(VM *vm)
{
  StackPush(vm->env, vm);
  CollectGarbage(vm->stack);
  vm->env = StackPop(vm);
}

#define OneArg(a) \
  CheckStack(vm, 1);\
  a = StackPop(vm)

#define TwoArgs(a, b) \
  CheckStack(vm, 2);\
  b = StackPop(vm);\
  a = StackPop(vm)

#define ThreeArgs(a, b, c) \
  CheckStack(vm, 3);\
  c = StackPop(vm);\
  b = StackPop(vm);\
  a = StackPop(vm)

#define UnaryOp(op)     StackPush(IntVal(op RawVal(a)), vm)
#define BinOp(op)       StackPush(IntVal(RawVal(a) op RawVal(b)), vm)
#define CheckBounds(n)  if ((n) < 0 || (n) > VecCount(vm->mod->code)) return outOfBounds

static VMStatus OpNoop(VM *vm)
{
  return vmOk;
}

static VMStatus OpConst(VM *vm)
{
  i32 num = ReadInt(&vm->pc, vm->mod);
  StackPush(vm->mod->constants[num], vm);
  return vmOk;
}

static VMStatus OpAdd(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(+);
  return vmOk;
}

static VMStatus OpSub(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(-);
  return vmOk;
}

static VMStatus OpMul(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(*);
  return vmOk;
}

static VMStatus OpDiv(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  if (RawVal(b) == 0) return divideByZero;
  BinOp(/);
  return vmOk;
}

static VMStatus OpRem(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  if (RawVal(b) == 0) return divideByZero;
  BinOp(%);
  return vmOk;
}

static VMStatus OpAnd(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(&);
  return vmOk;
}

static VMStatus OpOr(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(|);
  return vmOk;
}

static VMStatus OpComp(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsInt(a)) return invalidType;
  UnaryOp(~);
  return vmOk;
}

static VMStatus OpLt(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(<);
  return vmOk;
}

static VMStatus OpGt(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(>);
  return vmOk;
}

static VMStatus OpEq(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  StackPush(IntVal(a == b), vm);
  return vmOk;
}

static VMStatus OpNeg(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsInt(a)) return invalidType;
  UnaryOp(-);
  return vmOk;
}

static VMStatus OpNot(VM *vm)
{
  val a;
  OneArg(a);
  StackPush(IntVal(RawVal(a) != 0), vm);
  return vmOk;
}

static VMStatus OpShift(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(<<);
  return vmOk;
}

static VMStatus OpNil(VM *vm)
{
  StackPush(0, vm);
  return vmOk;
}

static VMStatus OpPair(VM *vm)
{
  val a, b;
  if (MemFree() < 2) RunGC(vm);
  TwoArgs(a, b);
  StackPush(Pair(b, a), vm);
  return vmOk;
}

static VMStatus OpHead(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsPair(a)) return invalidType;
  StackPush(Head(a), vm);
  return vmOk;
}

static VMStatus OpTail(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsPair(a)) return invalidType;
  StackPush(Tail(a), vm);
  return vmOk;
}

static VMStatus OpTuple(VM *vm)
{
  u32 count = ReadInt(&vm->pc, vm->mod);
  if (MemFree() < count+1) RunGC(vm);
  StackPush(Tuple(count), vm);
  return vmOk;
}

static VMStatus OpLen(VM *vm)
{
  val a;
  OneArg(a);
  if (IsPair(a)) {
    StackPush(IntVal(ListLength(a)), vm);
  } else if (IsTuple(a)) {
    StackPush(IntVal(TupleLength(a)), vm);
  } else if (IsBinary(a)) {
    StackPush(IntVal(BinaryLength(a)), vm);
  } else {
    return invalidType;
  }
  return vmOk;
}

static VMStatus OpGet(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
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
  return vmOk;
}

static VMStatus OpSet(VM *vm)
{
  val a, b, c;
  ThreeArgs(a, b, c);
  if (!IsInt(b)) return invalidType;
  if (IsTuple(a)) {
    TupleSet(a, RawVal(b), c);
  } else if (IsBinary(a)) {
    BinarySet(a, RawVal(b), c);
  } else {
    return invalidType;
  }
  return vmOk;
}

static VMStatus OpStr(VM *vm)
{
  char *name;
  u32 len;
  val a;
  OneArg(a);
  if (!IsSym(a)) return invalidType;
  name = SymbolName(RawVal(a));
  len = strlen(name);
  if (MemFree() < BinSpace(len) + 1) RunGC(vm);
  StackPush(BinaryFrom(name, len), vm);
  return vmOk;
}

static VMStatus OpBin(VM *vm)
{
  u32 count = ReadInt(&vm->pc, vm->mod);
  if (MemFree() < BinSpace(count) + 1) RunGC(vm);
  StackPush(Binary(count), vm);
  return vmOk;
}

static VMStatus OpJoin(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (ValType(a) != ValType(b)) return invalidType;
  if (IsPair(a)) {
    if (MemFree() < 2*(ListLength(a) + ListLength(b))) {
      StackPush(a, vm);
      StackPush(b, vm);
      RunGC(vm);
      TwoArgs(a, b);
    }
    StackPush(ListJoin(a, b), vm);
  } else if (IsTuple(a)) {
    if (MemFree() < 1 + TupleLength(a) + TupleLength(b)) {
      StackPush(a, vm);
      StackPush(b, vm);
      RunGC(vm);
      TwoArgs(a, b);
    }
    StackPush(TupleJoin(a, b), vm);
  } else if (IsBinary(a)) {
    if (MemFree() < 1 + BinSpace(BinaryLength(a)) + BinSpace(BinaryLength(b))) {
      StackPush(a, vm);
      StackPush(b, vm);
      RunGC(vm);
      TwoArgs(a, b);
    }
    StackPush(BinaryJoin(a, b), vm);
  } else {
    return invalidType;
  }
  return vmOk;
}

static VMStatus OpTrunc(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(b)) return invalidType;
  if (IsPair(a)) {
    if (MemFree() < 4*RawVal(b)) {
      StackPush(a, vm);
      RunGC(vm);
      a = StackPop(vm);
    }
    StackPush(ListTrunc(a, RawVal(b)), vm);
  } else if (IsTuple(a)) {
    if (MemFree() < RawVal(b)+1) {
      StackPush(a, vm);
      RunGC(vm);
      a = StackPop(vm);
    }
    StackPush(TupleTrunc(a, RawVal(b)), vm);
  } else if (IsBinary(a)) {
    if (MemFree() < BinSpace(RawVal(b))+1) {
      StackPush(a, vm);
      RunGC(vm);
      a = StackPop(vm);
    }
    StackPush(BinaryTrunc(a, RawVal(b)), vm);
  } else {
    return invalidType;
  }
  return vmOk;
}

static VMStatus OpSkip(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(b)) return invalidType;
  if (IsPair(a)) {
    StackPush(ListSkip(a, RawVal(b)), vm);
  } else if (IsTuple(a)) {
    if (MemFree() < TupleLength(a) - RawVal(b) + 1) {
      StackPush(a, vm);
      RunGC(vm);
      a = StackPop(vm);
    }
    StackPush(TupleSkip(a, RawVal(b)), vm);
  } else if (IsBinary(a)) {
    if (MemFree() < BinSpace(BinaryLength(a)) - RawVal(b) + 1) {
      StackPush(a, vm);
      RunGC(vm);
      a = StackPop(vm);
    }
    StackPush(BinarySkip(a, RawVal(b)), vm);
  } else {
    return invalidType;
  }
  return vmOk;
}

static VMStatus OpJmp(VM *vm)
{
  i32 n = ReadInt(&vm->pc, vm->mod);
  CheckBounds(vm->pc + n);
  vm->pc += n;
  return vmOk;
}

static VMStatus OpBr(VM *vm)
{
  i32 n = ReadInt(&vm->pc, vm->mod);
  val a;
  OneArg(a);
  if (RawVal(a)) {
    CheckBounds(vm->pc + n);
    vm->pc += n;
  }
  return vmOk;
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
  return vmOk;
}

static VMStatus OpGoto(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsInt(a)) return invalidType;
  CheckBounds(RawVal(a));
  vm->pc = RawVal(a);
  return vmOk;
}

static VMStatus OpHalt(VM *vm)
{
  vm->pc = VecCount(vm->mod->code);
  return vmOk;
}

static VMStatus OpDup(VM *vm)
{
  val a;
  OneArg(a);
  StackPush(a, vm);
  StackPush(a, vm);
  return vmOk;
}

static VMStatus OpDrop(VM *vm)
{
  if (VecCount(vm->stack) < 1) return stackUnderflow;
  VecPop(vm->stack);
  return vmOk;
}

static VMStatus OpSwap(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  StackPush(b, vm);
  StackPush(a, vm);
  return vmOk;
}

static VMStatus OpOver(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  StackPush(a, vm);
  StackPush(b, vm);
  StackPush(a, vm);
  return vmOk;
}

static VMStatus OpRot(VM *vm)
{
  val a, b, c;
  ThreeArgs(a, b, c);
  StackPush(b, vm);
  StackPush(c, vm);
  StackPush(a, vm);
  return vmOk;
}

static VMStatus OpGetEnv(VM *vm)
{
  StackPush(vm->env, vm);
  return vmOk;
}

static VMStatus OpSetEnv(VM *vm)
{
  if (VecCount(vm->stack) < 1) return stackUnderflow;
  vm->env = StackPop(vm);
  return vmOk;
}

static VMStatus OpLookup(VM *vm)
{
  i32 n = ReadInt(&vm->pc, vm->mod);
  StackPush(Lookup(n, vm->env), vm);
  return vmOk;
}

static VMStatus OpDefine(VM *vm)
{
  i32 n = ReadInt(&vm->pc, vm->mod);
  val a;
  OneArg(a);
  if (!Define(a, n, vm->env)) return outOfBounds;
  return vmOk;
}

void TraceInst(VM *vm)
{
  u32 index = vm->pc;
  char *op = DisassembleOp(&index, 0, vm->mod);
  u32 i, len = strlen(op);
  debug("%s ", op);
  for (i = 0; i < 20 - len; i++) debug(" ");
}

u32 PrintStack(VM *vm)
{
  u32 i, printed = 0;
  char *str;
  for (i = 0; i < 3; i++) {
    if (i >= VecCount(vm->stack)) break;
    str = ValStr(vm->stack[VecCount(vm->stack) - i - 1]);
    printed += debug("%s ", str);
    free(str);
  }
  if (VecCount(vm->stack) > 3) printed += debug("...");
  for (i = 0; (i32)i < 30 - (i32)printed; i++) debug(" ");
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
