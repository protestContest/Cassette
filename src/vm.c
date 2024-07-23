#include "vm.h"
#include "env.h"
#include "result.h"
#include "leb.h"
#include "primitives.h"
#include <univ/math.h>
#include <univ/str.h>
#include <univ/symbol.h>
#include <univ/vec.h>

typedef StatusCode (*OpFn)(VM *vm);

static void TraceInst(VM *vm);
static u32 PrintStack(VM *vm);
static PrimFn *InitPrimitives(void);

static StatusCode OpNoop(VM *vm);
static StatusCode OpConst(VM *vm);
static StatusCode OpAdd(VM *vm);
static StatusCode OpSub(VM *vm);
static StatusCode OpMul(VM *vm);
static StatusCode OpDiv(VM *vm);
static StatusCode OpRem(VM *vm);
static StatusCode OpAnd(VM *vm);
static StatusCode OpOr(VM *vm);
static StatusCode OpComp(VM *vm);
static StatusCode OpLt(VM *vm);
static StatusCode OpGt(VM *vm);
static StatusCode OpEq(VM *vm);
static StatusCode OpNeg(VM *vm);
static StatusCode OpNot(VM *vm);
static StatusCode OpShift(VM *vm);
static StatusCode OpPair(VM *vm);
static StatusCode OpHead(VM *vm);
static StatusCode OpTail(VM *vm);
static StatusCode OpTuple(VM *vm);
static StatusCode OpLen(VM *vm);
static StatusCode OpGet(VM *vm);
static StatusCode OpSet(VM *vm);
static StatusCode OpStr(VM *vm);
static StatusCode OpJoin(VM *vm);
static StatusCode OpSlice(VM *vm);
static StatusCode OpJmp(VM *vm);
static StatusCode OpBranch(VM *vm);
static StatusCode OpTrap(VM *vm);
static StatusCode OpPos(VM *vm);
static StatusCode OpGoto(VM *vm);
static StatusCode OpHalt(VM *vm);
static StatusCode OpDup(VM *vm);
static StatusCode OpDrop(VM *vm);
static StatusCode OpSwap(VM *vm);
static StatusCode OpOver(VM *vm);
static StatusCode OpRot(VM *vm);
static StatusCode OpGetEnv(VM *vm);
static StatusCode OpSetEnv(VM *vm);
static StatusCode OpGetMod(VM *vm);
static StatusCode OpSetMod(VM *vm);

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
  [opPair]    = OpPair,
  [opHead]    = OpHead,
  [opTail]    = OpTail,
  [opTuple]   = OpTuple,
  [opLen]     = OpLen,
  [opGet]     = OpGet,
  [opSet]     = OpSet,
  [opStr]     = OpStr,
  [opJoin]    = OpJoin,
  [opSlice]   = OpSlice,
  [opJump]    = OpJmp,
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
  [opGetMod]  = OpGetMod,
  [opSetMod]  = OpSetMod,
};

void InitVM(VM *vm, Program *program)
{
  vm->status = ok;
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

StatusCode VMStep(VM *vm)
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

void VMRun(Program *program)
{
  VM vm;
  InitVM(&vm, program);
  while (!VMDone(&vm)) {
    VMStep(&vm);
  }
  if (vm.status != ok) {
    fprintf(stderr, "Error!\n");
  }
}

void VMDebug(Program *program)
{
  VM vm;
  InitVM(&vm, program);
  InitMem(1024);

  fprintf(stderr, "───┬─inst─────────stack───────────────\n");

  while (!VMDone(&vm)) {
    VMTrace(&vm, 0);
    VMStep(&vm);
  }
  if (vm.status != ok) {
    fprintf(stderr, "Error!\n");
  }
}

val VMStackPop(VM *vm)
{
  return VecPop(vm->stack);
}

void VMStackPush(val value, VM *vm)
{
  VecPush(vm->stack, value);
}

void RunGC(VM *vm)
{
  VMStackPush(vm->env, vm);
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

#define UnaryOp(op)     VMStackPush(IntVal(op RawVal(a)), vm)
#define BinOp(op)       VMStackPush(IntVal(RawVal(a) op RawVal(b)), vm)
#define CheckBounds(n) \
  if ((n) < 0 || (n) > (i32)VecCount(vm->program->code)) return outOfBounds

static StatusCode OpNoop(VM *vm)
{
  return ok;
}

static StatusCode OpConst(VM *vm)
{
  val value = ReadLEB(vm->pc, vm->program->code);
  vm->pc += LEBSize(value);
  VMStackPush(value, vm);
  return ok;
}

static StatusCode OpAdd(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(+);
  return ok;
}

static StatusCode OpSub(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(-);
  return ok;
}

static StatusCode OpMul(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(*);
  return ok;
}

static StatusCode OpDiv(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  if (RawVal(b) == 0) return divideByZero;
  BinOp(/);
  return ok;
}

static StatusCode OpRem(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  if (RawVal(b) == 0) return divideByZero;
  BinOp(%);
  return ok;
}

static StatusCode OpAnd(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(&);
  return ok;
}

static StatusCode OpOr(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(|);
  return ok;
}

static StatusCode OpComp(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsInt(a)) return invalidType;
  UnaryOp(~);
  return ok;
}

static StatusCode OpLt(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(<);
  return ok;
}

static StatusCode OpGt(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(>);
  return ok;
}

static StatusCode OpEq(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  VMStackPush(IntVal(a == b), vm);
  return ok;
}

static StatusCode OpNeg(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsInt(a)) return invalidType;
  UnaryOp(-);
  return ok;
}

static StatusCode OpNot(VM *vm)
{
  val a;
  OneArg(a);
  VMStackPush(IntVal(RawVal(a) != 0), vm);
  return ok;
}

static StatusCode OpShift(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return invalidType;
  BinOp(<<);
  return ok;
}

static StatusCode OpPair(VM *vm)
{
  val a, b;
  if (MemFree() < 2) RunGC(vm);
  TwoArgs(a, b);
  VMStackPush(Pair(b, a), vm);
  return ok;
}

static StatusCode OpHead(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsPair(a)) return invalidType;
  VMStackPush(Head(a), vm);
  return ok;
}

static StatusCode OpTail(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsPair(a)) return invalidType;
  VMStackPush(Tail(a), vm);
  return ok;
}

static StatusCode OpTuple(VM *vm)
{
  u32 count = ReadLEB(vm->pc, vm->program->code);
  vm->pc += LEBSize(count);
  if (MemFree() < count+1) RunGC(vm);
  VMStackPush(Tuple(count), vm);
  return ok;
}

static StatusCode OpLen(VM *vm)
{
  val a;
  OneArg(a);
  if (IsPair(a)) {
    VMStackPush(IntVal(ListLength(a)), vm);
  } else if (IsTuple(a)) {
    VMStackPush(IntVal(TupleLength(a)), vm);
  } else if (IsBinary(a)) {
    VMStackPush(IntVal(BinaryLength(a)), vm);
  } else {
    return invalidType;
  }
  return ok;
}

static StatusCode OpGet(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(b)) return invalidType;
  if (IsPair(a)) {
    if (RawInt(b) < 0 || RawInt(b) >= (i32)ListLength(a)) return outOfBounds;
    VMStackPush(ListGet(a, RawInt(b)), vm);
  } else if (IsTuple(a)) {
    if (RawInt(b) < 0 || RawInt(b) >= (i32)TupleLength(a)) return outOfBounds;
    VMStackPush(TupleGet(a, RawInt(b)), vm);
  } else if (IsBinary(a)) {
    if (RawInt(b) < 0 || RawInt(b) >= (i32)BinaryLength(a)) return outOfBounds;
    VMStackPush(BinaryGet(a, RawInt(b)), vm);
  } else {
    return invalidType;
  }
  return ok;
}

static StatusCode OpSet(VM *vm)
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
  return ok;
}

static StatusCode OpStr(VM *vm)
{
  char *name;
  u32 len;
  val a;
  OneArg(a);
  if (!IsSym(a)) return invalidType;
  name = SymbolName(RawVal(a));
  len = strlen(name);
  if (MemFree() < BinSpace(len) + 1) RunGC(vm);
  VMStackPush(BinaryFrom(name, len), vm);
  return ok;
}

static StatusCode OpJoin(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (ValType(a) != ValType(b)) return invalidType;
  if (IsPair(a)) {
    if (MemFree() < 2*(ListLength(a) + ListLength(b))) {
      VMStackPush(a, vm);
      VMStackPush(b, vm);
      RunGC(vm);
      TwoArgs(a, b);
    }
    VMStackPush(ListJoin(a, b), vm);
  } else if (IsTuple(a)) {
    if (MemFree() < 1 + TupleLength(a) + TupleLength(b)) {
      VMStackPush(a, vm);
      VMStackPush(b, vm);
      RunGC(vm);
      TwoArgs(a, b);
    }
    VMStackPush(TupleJoin(a, b), vm);
  } else if (IsBinary(a)) {
    if (MemFree() < 1 + BinSpace(BinaryLength(a)) + BinSpace(BinaryLength(b))) {
      VMStackPush(a, vm);
      VMStackPush(b, vm);
      RunGC(vm);
      TwoArgs(a, b);
    }
    VMStackPush(BinaryJoin(a, b), vm);
  } else {
    return invalidType;
  }
  return ok;
}

static StatusCode OpSlice(VM *vm)
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
        VMStackPush(list, vm);
        RunGC(vm);
        list = VMStackPop(vm);
      }
      list = ListTrunc(list, RawVal(c) - RawVal(b));
    }
    VMStackPush(list, vm);
  } else if (IsTuple(a)) {
    u32 len;
    if (!c) c = IntVal(TupleLength(a));
    if (RawInt(c) < 0 || RawInt(c) > (i32)TupleLength(a)) return outOfBounds;
    len = RawVal(c) - RawVal(b);
    if (MemFree() < len+1) {
      VMStackPush(a, vm);
      RunGC(vm);
      a = VMStackPop(vm);
    }
    VMStackPush(TupleSlice(a, RawVal(b), RawVal(c)), vm);
  } else if (IsBinary(a)) {
    u32 len;
    if (!c) c = IntVal(BinaryLength(a));
    if (RawInt(c) < 0 || RawInt(c) > (i32)BinaryLength(a)) return outOfBounds;
    len = RawVal(c) - RawVal(b);
    if (MemFree() < BinSpace(len)+1) {
      VMStackPush(a, vm);
      RunGC(vm);
      a = VMStackPop(vm);
    }
    VMStackPush(BinarySlice(a, RawVal(b), RawVal(c)), vm);
  } else {
    return invalidType;
  }
  return ok;
}

static StatusCode OpJmp(VM *vm)
{
  i32 n = ReadLEB(vm->pc, vm->program->code);
  vm->pc += LEBSize(n);
  CheckBounds((i32)vm->pc + n);
  vm->pc += n;
  return ok;
}

static StatusCode OpBranch(VM *vm)
{
  val a;
  i32 n = ReadLEB(vm->pc, vm->program->code);
  vm->pc += LEBSize(n);
  OneArg(a);
  if (RawVal(a)) {
    CheckBounds((i32)vm->pc + n);
    vm->pc += n;
  }
  return ok;
}

static StatusCode OpTrap(VM *vm)
{
  Result result;
  u32 id = ReadLEB(vm->pc, vm->program->code);
  vm->pc += LEBSize(id);
  result = vm->primitives[id](vm);
  vm->status = result.status;
  if (!IsError(result)) VMStackPush(result.data.v, vm);
  VMStackPush(result.data.v, vm);
  return vm->status;
}

static StatusCode OpPos(VM *vm)
{
  i32 n = ReadLEB(vm->pc, vm->program->code);
  vm->pc += LEBSize(n);
  VMStackPush(IntVal((i32)vm->pc + n), vm);
  return ok;
}

static StatusCode OpGoto(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsInt(a)) return invalidType;
  CheckBounds(RawInt(a));
  vm->pc = RawVal(a);
  return ok;
}

static StatusCode OpHalt(VM *vm)
{
  vm->pc = VecCount(vm->program->code);
  return ok;
}

static StatusCode OpDup(VM *vm)
{
  val a;
  OneArg(a);
  VMStackPush(a, vm);
  VMStackPush(a, vm);
  return ok;
}

static StatusCode OpDrop(VM *vm)
{
  if (VecCount(vm->stack) < 1) return stackUnderflow;
  VecPop(vm->stack);
  return ok;
}

static StatusCode OpSwap(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  VMStackPush(b, vm);
  VMStackPush(a, vm);
  return ok;
}

static StatusCode OpOver(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  VMStackPush(a, vm);
  VMStackPush(b, vm);
  VMStackPush(a, vm);
  return ok;
}

static StatusCode OpRot(VM *vm)
{
  val a, b, c;
  ThreeArgs(a, b, c);
  VMStackPush(b, vm);
  VMStackPush(c, vm);
  VMStackPush(a, vm);
  return ok;
}

static StatusCode OpGetEnv(VM *vm)
{
  VMStackPush(vm->env, vm);
  return ok;
}

static StatusCode OpSetEnv(VM *vm)
{
  if (VecCount(vm->stack) < 1) return stackUnderflow;
  vm->env = VMStackPop(vm);
  return ok;
}

static StatusCode OpGetMod(VM *vm)
{
  VMStackPush(vm->mod, vm);
  return ok;
}

static StatusCode OpSetMod(VM *vm)
{
  val a;
  OneArg(a);
  vm->mod = a;
  return ok;
}

void VMTrace(VM *vm, char *src)
{
  TraceInst(vm);
  PrintStack(vm);
  printf("\n");
}

static void TraceInst(VM *vm)
{
  u32 index = vm->pc;
  u32 i, len;
  len = DisassembleInst(vm->program->code, &index);
  for (i = 0; i < 20 - len; i++) fprintf(stderr, " ");
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

#define RuntimeError(msg, pos) Error(Binary(msg), 0, 0, 0)

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
