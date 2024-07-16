#include "vm.h"
#include "env.h"
#include "error.h"
#include "leb.h"
#include "primitives.h"
#include <univ/math.h>
#include <univ/str.h>
#include <univ/symbol.h>
#include <univ/vec.h>

typedef VMStatus (*OpFn)(VM *vm);

static void TraceInst(VM *vm);
static u32 PrintStack(VM *vm);
static PrimFn *InitPrimitives(void);

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
static VMStatus OpSlice(VM *vm);
static VMStatus OpJmp(VM *vm);
static VMStatus OpBranch(VM *vm);
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
  [opSlice]   = OpSlice,
  [opJmp]     = OpJmp,
  [opBranch]  = OpBranch,
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

void InitVM(VM *vm, Program *program)
{
  vm->status = vmOk;
  vm->pc = 0;
  vm->env = 0;
  vm->stack = 0;
  vm->program = program;
  if (program) ImportSymbols(program->symbols, VecCount(program->symbols));
  vm->primitives = InitPrimitives();
}

void DestroyVM(VM *vm)
{
  FreeVec(vm->stack);
  free(vm->primitives);
}

VMStatus VMStep(VM *vm)
{
  vm->status = ops[vm->program->code[vm->pc++]](vm);
  return vm->status;
}

void PrintSourceFrom(u32 index, char *src)
{
  char *start = src+index;
  char *end = LineEnd(index, src);
  if (end > start && end[-1] == '\n') end--;
  fprintf(stderr, "%*.*s", (i32)(end-start), (i32)(end-start), start);
}

void VMRun(Program *program, VM *vm)
{
  InitVM(vm, program);
  while (!VMDone(vm)) {
    VMStep(vm);
  }
}

val VMStackPop(VM *vm)
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
  vm->env = VMStackPop(vm);
}

#define OneArg(a) \
  CheckStack(vm, 1);\
  a = VMStackPop(vm)

#define TwoArgs(a, b) \
  CheckStack(vm, 2);\
  b = VMStackPop(vm);\
  a = VMStackPop(vm)

#define ThreeArgs(a, b, c) \
  CheckStack(vm, 3);\
  c = VMStackPop(vm);\
  b = VMStackPop(vm);\
  a = VMStackPop(vm)

#define UnaryOp(op)     StackPush(IntVal(op RawVal(a)), vm)
#define BinOp(op)       StackPush(IntVal(RawVal(a) op RawVal(b)), vm)
#define CheckBounds(n) \
  if ((n) < 0 || (n) > (i32)VecCount(vm->program->code)) return outOfBounds

static VMStatus OpNoop(VM *vm)
{
  return vmOk;
}

static VMStatus OpConst(VM *vm)
{
  val value = ReadLEB(vm->pc, vm->program->code);
  vm->pc += LEBSize(value);
  VMStackPush(value, vm);
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
  u32 count = ReadLEB(vm->pc, vm->program->code);
  vm->pc += LEBSize(count);
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
    if (RawInt(b) < 0 || RawInt(b) >= (i32)ListLength(a)) return outOfBounds;
    StackPush(ListGet(a, RawInt(b)), vm);
  } else if (IsTuple(a)) {
    if (RawInt(b) < 0 || RawInt(b) >= (i32)TupleLength(a)) return outOfBounds;
    StackPush(TupleGet(a, RawInt(b)), vm);
  } else if (IsBinary(a)) {
    if (RawInt(b) < 0 || RawInt(b) >= (i32)BinaryLength(a)) return outOfBounds;
    StackPush(BinaryGet(a, RawInt(b)), vm);
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
  if (RawInt(b) < 0) return outOfBounds;
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
  u32 count = ReadLEB(vm->pc, vm->program->code);
  vm->pc += LEBSize(count);
  if (MemFree() < BinSpace(count) + 1) RunGC(vm);
  StackPush(NewBinary(count), vm);
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

static VMStatus OpSlice(VM *vm)
{
  val a, b, c;
  ThreeArgs(a, b, c);
  if (!IsInt(b)) return invalidType;
  if (RawInt(b) < 0) return outOfBounds;
  if (c && !IsInt(c)) return invalidType;

  if (IsPair(a)) {
    val list = ListSkip(a, RawVal(b));
    if (c) {
      u32 len;
      if (RawInt(c) < 0 || RawInt(c) > (i32)ListLength(a)) return outOfBounds;
      len = RawVal(c) - RawVal(b);
      if (MemFree() < 4*len) {
        StackPush(list, vm);
        RunGC(vm);
        list = VMStackPop(vm);
      }
      list = ListTrunc(list, RawVal(c) - RawVal(b));
    }
    StackPush(list, vm);
  } else if (IsTuple(a)) {
    u32 len;
    if (!c) c = IntVal(TupleLength(a));
    if (RawInt(c) < 0 || RawInt(c) > (i32)TupleLength(a)) return outOfBounds;
    len = RawVal(c) - RawVal(b);
    if (MemFree() < len+1) {
      StackPush(a, vm);
      RunGC(vm);
      a = VMStackPop(vm);
    }
    StackPush(TupleSlice(a, RawVal(b), RawVal(c)), vm);
  } else if (IsBinary(a)) {
    u32 len;
    if (!c) c = IntVal(BinaryLength(a));
    if (RawInt(c) < 0 || RawInt(c) > (i32)BinaryLength(a)) return outOfBounds;
    len = RawVal(c) - RawVal(b);
    if (MemFree() < BinSpace(len)+1) {
      StackPush(a, vm);
      RunGC(vm);
      a = VMStackPop(vm);
    }
    StackPush(BinarySlice(a, RawVal(b), RawVal(c)), vm);
  } else {
    return invalidType;
  }
  return vmOk;
}

static VMStatus OpJmp(VM *vm)
{
  i32 n = ReadLEB(vm->pc, vm->program->code);
  vm->pc += LEBSize(n);
  CheckBounds((i32)vm->pc + n);
  vm->pc += n;
  return vmOk;
}

static VMStatus OpBranch(VM *vm)
{
  val a;
  i32 n = ReadLEB(vm->pc, vm->program->code);
  vm->pc += LEBSize(n);
  OneArg(a);
  if (RawVal(a)) {
    CheckBounds((i32)vm->pc + n);
    vm->pc += n;
  }
  return vmOk;
}

static VMStatus OpTrap(VM *vm)
{
  u32 id = ReadLEB(vm->pc, vm->program->code);
  vm->pc += LEBSize(id);
  return vm->primitives[id](vm);
}

static VMStatus OpPos(VM *vm)
{
  i32 n = ReadLEB(vm->pc, vm->program->code);
  vm->pc += LEBSize(n);
  StackPush(IntVal((i32)vm->pc + n), vm);
  return vmOk;
}

static VMStatus OpGoto(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsInt(a)) return invalidType;
  CheckBounds(RawInt(a));
  vm->pc = RawVal(a);
  return vmOk;
}

static VMStatus OpHalt(VM *vm)
{
  vm->pc = VecCount(vm->program->code);
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
  vm->env = VMStackPop(vm);
  return vmOk;
}

static VMStatus OpLookup(VM *vm)
{
  i32 n = ReadLEB(vm->pc, vm->program->code);
  vm->pc += LEBSize(n);
  VMStackPush(EnvGet(n, vm->env), vm);
  return vmOk;
}

static VMStatus OpDefine(VM *vm)
{
  val a;
  i32 n = ReadLEB(vm->pc, vm->program->code);
  vm->pc += LEBSize(n);
  OneArg(a);
  if (!EnvSet(a, n, vm->env)) return outOfBounds;
  return vmOk;
}

void VMTrace(VM *vm, char *src)
{
  TraceInst(vm);
  PrintStack(vm);
  printf("\n");
}

static void TraceInst(VM *vm)
{
  /*
  u32 index = vm->pc;
  char *op = DisassembleOp(&index, 0, vm->mod);
  u32 i, len = strlen(op);
  fprintf(stderr, "%s ", op);
  for (i = 0; i < 20 - len; i++) fprintf(stderr, " ");
  FreeVec(op);
  */
}

static u32 PrintStack(VM *vm)
{
  u32 i, printed = 0, max = 6;
  char *str;
  for (i = 0; i < max; i++) {
    if (i >= VecCount(vm->stack)) break;
    str = MemValStr(vm->stack[VecCount(vm->stack) - i - 1]);
    printed += fprintf(stderr, "%s ", str);
    free(str);
  }
  if (VecCount(vm->stack) > max) printed += fprintf(stderr, "...");
  for (i = 0; (i32)i < 30 - (i32)printed; i++) fprintf(stderr, " ");
  return printed;
}

#define RuntimeError(msg, pos) MakeError(Binary(msg), Pair(pos, pos))

val VMError(VM *vm)
{
  switch (vm->status) {
  case stackUnderflow:
    return RuntimeError("Stack Underflow", 0);
  case invalidType:
    return RuntimeError("Invalid Type", 0);
  case divideByZero:
    return RuntimeError("Divdie by Zero", 0);
  case outOfBounds:
    return RuntimeError("Out of Bounds", 0);
  case unhandledTrap:
    return RuntimeError("Trap", 0);
  default:
    return nil;
  }
}

static PrimFn *InitPrimitives(void)
{
  PrimDef *primitives = Primitives();
  PrimFn *fns = malloc(sizeof(PrimFn)*NumPrimitives());
  u32 i;
  for (i = 0; i < NumPrimitives(); i++) {
    fns[i] = primitives[i].fn;
  }
  return fns;
}
