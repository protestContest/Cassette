#include "vm.h"
#include "env.h"
#include "result.h"
#include "leb.h"
#include "primitives.h"
#include <univ/math.h>
#include <univ/str.h>
#include <univ/symbol.h>
#include <univ/vec.h>
#include <SDL2/SDL.h>

typedef Result (*OpFn)(VM *vm);

static void TraceInst(VM *vm);
static u32 PrintStack(VM *vm);
static PrimFn *InitPrimitives(void);

static Result OpNoop(VM *vm);
static Result OpHalt(VM *vm);
static Result OpConst(VM *vm);
static Result OpJump(VM *vm);
static Result OpBranch(VM *vm);
static Result OpPos(VM *vm);
static Result OpGoto(VM *vm);
static Result OpGetEnv(VM *vm);
static Result OpSetEnv(VM *vm);
static Result OpGetMod(VM *vm);
static Result OpSetMod(VM *vm);
static Result OpAdd(VM *vm);
static Result OpSub(VM *vm);
static Result OpMul(VM *vm);
static Result OpDiv(VM *vm);
static Result OpRem(VM *vm);
static Result OpAnd(VM *vm);
static Result OpOr(VM *vm);
static Result OpComp(VM *vm);
static Result OpLt(VM *vm);
static Result OpGt(VM *vm);
static Result OpEq(VM *vm);
static Result OpNeg(VM *vm);
static Result OpNot(VM *vm);
static Result OpShift(VM *vm);
static Result OpDup(VM *vm);
static Result OpDrop(VM *vm);
static Result OpSwap(VM *vm);
static Result OpOver(VM *vm);
static Result OpRot(VM *vm);
static Result OpPick(VM *vm);
static Result OpRoll(VM *vm);
static Result OpPair(VM *vm);
static Result OpHead(VM *vm);
static Result OpTail(VM *vm);
static Result OpTuple(VM *vm);
static Result OpLen(VM *vm);
static Result OpGet(VM *vm);
static Result OpSet(VM *vm);
static Result OpStr(VM *vm);
static Result OpJoin(VM *vm);
static Result OpSlice(VM *vm);
static Result OpTrap(VM *vm);

static OpFn ops[] = {
  [opNoop]    = OpNoop,
  [opHalt]    = OpHalt,
  [opConst]   = OpConst,
  [opJump]    = OpJump,
  [opBranch]  = OpBranch,
  [opPos]     = OpPos,
  [opGoto]    = OpGoto,
  [opGetEnv]  = OpGetEnv,
  [opSetEnv]  = OpSetEnv,
  [opGetMod]  = OpGetMod,
  [opSetMod]  = OpSetMod,
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
  [opDup]     = OpDup,
  [opDrop]    = OpDrop,
  [opSwap]    = OpSwap,
  [opOver]    = OpOver,
  [opRot]     = OpRot,
  [opPick]    = OpPick,
  [opRoll]    = OpRoll,
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
  [opTrap]    = OpTrap,
};

Result RuntimeError(char *message, struct VM *vm)
{
  char *file = GetSourceFile(vm->pc, &vm->program->srcmap);
  u32 pos = GetSourcePos(vm->pc, &vm->program->srcmap);
  Error *error = NewError(0, file, pos, 0);
  error->message = StrCat(error->message, "Runtime error: ");
  error->message = StrCat(error->message, message);
  return Err(error);
}

void InitVM(VM *vm, Program *program)
{
  vm->status = Ok(0);
  vm->pc = 0;
  vm->env = 0;
  vm->stack = 0;
  vm->program = program;
  if (program) ImportSymbols(program->strings, VecCount(program->strings));
  vm->primitives = InitPrimitives();
  vm->refs = 0;

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
}

void DestroyVM(VM *vm)
{
  FreeVec(vm->stack);
  FreeVec(vm->refs);
  free(vm->primitives);

  SDL_Quit();
}

Result VMStep(VM *vm)
{
  vm->status = ops[vm->program->code[vm->pc]](vm);
  return vm->status;
}

void PrintSourceFrom(u32 index, char *src)
{
  char *start = src+index;
  char *end = LineEnd(index, src);
  if (end > start && end[-1] == '\n') end--;
  fprintf(stderr, "%*.*s", (i32)(end-start), (i32)(end-start), start);
}

Result VMRun(Program *program)
{
  VM vm;

  InitVM(&vm, program);

#if DEBUG
  {
    u32 num_width = NumDigits(VecCount(program->code), 10);
    u32 i;
    for (i = 0; i < num_width; i++) fprintf(stderr, "─");
    fprintf(stderr, "┬─inst─────────stack───────────────\n");
  }
#endif

  while (!VMDone(&vm)) {
#if DEBUG
    VMTrace(&vm, 0);
#endif
    VMStep(&vm);
  }

  DestroyVM(&vm);
  return vm.status;
}

val VMStackPop(VM *vm)
{
  return VecPop(vm->stack);
}

void VMStackPush(val value, VM *vm)
{
  VecPush(vm->stack, value);
}

u32 VMPushRef(void *ref, VM *vm)
{
  u32 index = VecCount(vm->refs);
  VecPush(vm->refs, ref);
  return index;
}

void *VMGetRef(u32 ref, VM *vm)
{
  return vm->refs[ref];
}

void MaybeGC(u32 size, VM *vm)
{
  if (MemFree() < size) RunGC(vm);
}

void RunGC(VM *vm)
{
  VMStackPush(vm->env, vm);
  VMStackPush(vm->mod, vm);
  CollectGarbage(vm->stack);
  vm->mod = VMStackPop(vm);
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
  if ((n) < 0 || (n) > (i32)VecCount(vm->program->code)) \
    return RuntimeError("Out of bounds", vm)

static Result OpNoop(VM *vm)
{
  vm->pc++;
  return vm->status;
}

static Result OpHalt(VM *vm)
{
  vm->pc = VecCount(vm->program->code);
  return vm->status;
}

static Result OpConst(VM *vm)
{
  val value = ReadLEB(++vm->pc, vm->program->code);
  vm->pc += LEBSize(value);
  VMStackPush(value, vm);
  return vm->status;
}

static Result OpPos(VM *vm)
{
  i32 n = ReadLEB(++vm->pc, vm->program->code);
  vm->pc += LEBSize(n);
  VMStackPush(IntVal((i32)vm->pc + n), vm);
  return vm->status;
}

static Result OpGoto(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsInt(a)) return RuntimeError("Invalid type", vm);
  CheckBounds(RawInt(a));
  vm->pc = RawVal(a);
  return vm->status;
}

static Result OpJump(VM *vm)
{
  i32 n = ReadLEB(++vm->pc, vm->program->code);
  vm->pc += LEBSize(n);
  CheckBounds((i32)vm->pc + n);
  vm->pc += n;
  return vm->status;
}

static Result OpBranch(VM *vm)
{
  val a;
  i32 n = ReadLEB(++vm->pc, vm->program->code);
  vm->pc += LEBSize(n);
  OneArg(a);
  if (RawVal(a)) {
    CheckBounds((i32)vm->pc + n);
    vm->pc += n;
  }
  return vm->status;
}

static Result OpGetEnv(VM *vm)
{
  VMStackPush(vm->env, vm);
  vm->pc++;
  return vm->status;
}

static Result OpSetEnv(VM *vm)
{
  if (VecCount(vm->stack) < 1) return RuntimeError("Stack underflow", vm);
  vm->env = VMStackPop(vm);
  vm->pc++;
  return vm->status;
}

static Result OpGetMod(VM *vm)
{
  VMStackPush(vm->mod, vm);
  vm->pc++;
  return vm->status;
}

static Result OpSetMod(VM *vm)
{
  val a;
  OneArg(a);
  vm->mod = a;
  vm->pc++;
  return vm->status;
}

static Result OpGetA(VM *vm)
{
  VMStackPush(vm->mod, vm);
  vm->pc++;
  return vm->status;
}

static Result OpSetA(VM *vm)
{
  val a;
  OneArg(a);
  vm->mod = a;
  vm->pc++;
  return vm->status;
}

static Result OpAdd(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be added", vm);
  BinOp(+);
  vm->pc++;
  return vm->status;
}

static Result OpSub(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be subtracted", vm);
  BinOp(-);
  vm->pc++;
  return vm->status;
}

static Result OpMul(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be multiplied", vm);
  BinOp(*);
  vm->pc++;
  return vm->status;
}

static Result OpDiv(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be divided", vm);
  if (RawVal(b) == 0) return RuntimeError("Divide by zero", vm);
  BinOp(/);
  vm->pc++;
  return vm->status;
}

static Result OpRem(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be remaindered", vm);
  if (RawVal(b) == 0) return RuntimeError("Divide by zero", vm);
  BinOp(%);
  vm->pc++;
  return vm->status;
}

static Result OpAnd(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be and-ed", vm);
  BinOp(&);
  vm->pc++;
  return vm->status;
}

static Result OpOr(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be or-ed", vm);
  BinOp(|);
  vm->pc++;
  return vm->status;
}

static Result OpComp(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsInt(a)) return RuntimeError("Only integers can be ", vm);
  UnaryOp(~);
  vm->pc++;
  return vm->status;
}

static Result OpLt(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be compared", vm);
  BinOp(<);
  vm->pc++;
  return vm->status;
}

static Result OpGt(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be compared", vm);
  BinOp(>);
  vm->pc++;
  return vm->status;
}

static Result OpEq(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  VMStackPush(IntVal(a == b), vm);
  vm->pc++;
  return vm->status;
}

static Result OpNeg(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsInt(a)) return RuntimeError("Only integers can be negated", vm);
  UnaryOp(-);
  vm->pc++;
  return vm->status;
}

static Result OpNot(VM *vm)
{
  val a;
  OneArg(a);
  VMStackPush(IntVal(RawVal(a) == 0), vm);
  vm->pc++;
  return vm->status;
}

static Result OpShift(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be shifted", vm);
  BinOp(<<);
  vm->pc++;
  return vm->status;
}

static Result OpDup(VM *vm)
{
  val a;
  OneArg(a);
  VMStackPush(a, vm);
  VMStackPush(a, vm);
  vm->pc++;
  return vm->status;
}

static Result OpDrop(VM *vm)
{
  if (VecCount(vm->stack) < 1) return RuntimeError("Stack underflow", vm);
  VecPop(vm->stack);
  vm->pc++;
  return vm->status;
}

static Result OpSwap(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  VMStackPush(b, vm);
  VMStackPush(a, vm);
  vm->pc++;
  return vm->status;
}

static Result OpOver(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  VMStackPush(a, vm);
  VMStackPush(b, vm);
  VMStackPush(a, vm);
  vm->pc++;
  return vm->status;
}

static Result OpRot(VM *vm)
{
  val a, b, c;
  ThreeArgs(a, b, c);
  VMStackPush(b, vm);
  VMStackPush(c, vm);
  VMStackPush(a, vm);
  vm->pc++;
  return vm->status;
}

static Result OpPick(VM *vm)
{
  u32 n = ReadLEB(++vm->pc, vm->program->code);
  val v;
  vm->pc += LEBSize(n);
  CheckStack(vm, n);
  if (n < 0) return RuntimeError("Invalid stack index", vm);

  v = vm->stack[VecCount(vm->stack) - 1 - n];
  VMStackPush(v, vm);
  return vm->status;
}

static Result OpRoll(VM *vm)
{
  u32 n = ReadLEB(++vm->pc, vm->program->code);
  u32 i;
  val v;
  vm->pc += LEBSize(n);
  CheckStack(vm, n);
  if (n < 0) return RuntimeError("Invalid stack index", vm);

  v = vm->stack[VecCount(vm->stack) - 1 - n];
  for (i = n; i > 0; i--) {
    vm->stack[VecCount(vm->stack) - 1 - i] = vm->stack[VecCount(vm->stack - i)];
  }
  vm->stack[VecCount(vm->stack - 1)] = v;

  return vm->status;
}

static Result OpPair(VM *vm)
{
  val a, b;
  MaybeGC(2, vm);
  TwoArgs(a, b);
  VMStackPush(Pair(b, a), vm);
  vm->pc++;
  return vm->status;
}

static Result OpHead(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsPair(a)) return RuntimeError("Only pairs have heads", vm);
  VMStackPush(Head(a), vm);
  vm->pc++;
  return vm->status;
}

static Result OpTail(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsPair(a)) return RuntimeError("Only pairs have tails", vm);
  VMStackPush(Tail(a), vm);
  vm->pc++;
  return vm->status;
}

static Result OpTuple(VM *vm)
{
  u32 count = ReadLEB(++vm->pc, vm->program->code);
  vm->pc += LEBSize(count);
  MaybeGC(count+1, vm);
  VMStackPush(Tuple(count), vm);
  return vm->status;
}

static Result OpLen(VM *vm)
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
    return RuntimeError("Invalid type", vm);
  }
  vm->pc++;
  return vm->status;
}

static Result OpGet(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(b)) return RuntimeError("Only integers can be indexes", vm);
  if (IsPair(a)) {
    if (RawInt(b) < 0 || RawInt(b) >= (i32)ListLength(a)) return RuntimeError("Out of bounds", vm);
    VMStackPush(ListGet(a, RawInt(b)), vm);
  } else if (IsTuple(a)) {
    if (RawInt(b) < 0 || RawInt(b) >= (i32)TupleLength(a)) return RuntimeError("Out of bounds", vm);
    VMStackPush(TupleGet(a, RawInt(b)), vm);
  } else if (IsBinary(a)) {
    if (RawInt(b) < 0 || RawInt(b) >= (i32)BinaryLength(a)) return RuntimeError("Out of bounds", vm);
    VMStackPush(IntVal(BinaryGet(a, RawInt(b))), vm);
  } else {
    return RuntimeError("Invalid type", vm);
  }
  vm->pc++;
  return vm->status;
}

static Result OpSet(VM *vm)
{
  val a, b, c;
  ThreeArgs(a, b, c);
  if (!IsInt(b)) return RuntimeError("Only integers can be indexes", vm);
  if (RawInt(b) < 0) return RuntimeError("Out of bounds", vm);
  if (IsTuple(a)) {
    TupleSet(a, RawVal(b), c);
  } else if (IsBinary(a)) {
    BinarySet(a, RawVal(b), c);
  } else {
    return RuntimeError("Invalid type", vm);
  }
  VMStackPush(a, vm);
  vm->pc++;
  return vm->status;
}

static Result OpStr(VM *vm)
{
  char *name;
  u32 len;
  val a;
  OneArg(a);
  if (!IsSym(a)) return RuntimeError("Only symbols can become strings", vm);
  name = SymbolName(RawVal(a));
  len = strlen(name);
  MaybeGC(BinSpace(len) + 1, vm);
  VMStackPush(BinaryFrom(name, len), vm);
  vm->pc++;
  return vm->status;
}

static Result OpJoin(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (ValType(a) != ValType(b)) return RuntimeError("Only values of the same type can be joined", vm);
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
    return RuntimeError("Only lists, tuples, and binaries can be joined", vm);
  }
  vm->pc++;
  return vm->status;
}

static Result OpSlice(VM *vm)
{
  val a, b, c;
  ThreeArgs(a, b, c);
  if (!IsInt(b)) return RuntimeError("Only integers can be slice indexes", vm);
  if (RawInt(b) < 0) return RuntimeError("Out of bounds", vm);
  if (c && !IsInt(c)) return RuntimeError("Only integers can be slice indexes", vm);

  if (IsPair(a)) {
    val list = ListSkip(a, RawVal(b));
    if (c) {
      u32 len;
      if (RawInt(c) < 0 || RawInt(c) > (i32)ListLength(a)) return RuntimeError("Out of bounds", vm);
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
    if (RawInt(c) < 0 || RawInt(c) > (i32)TupleLength(a)) return RuntimeError("Out of bounds", vm);
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
    if (RawInt(c) < 0 || RawInt(c) > (i32)BinaryLength(a)) return RuntimeError("Out of bounds", vm);
    len = RawVal(c) - RawVal(b);
    if (MemFree() < BinSpace(len)+1) {
      VMStackPush(a, vm);
      RunGC(vm);
      a = VMStackPop(vm);
    }
    VMStackPush(BinarySlice(a, RawVal(b), RawVal(c)), vm);
  } else {
    return RuntimeError("Invalid type", vm);
  }
  vm->pc++;
  return vm->status;
}

static Result OpTrap(VM *vm)
{
  u32 id = ReadLEB(vm->pc+1, vm->program->code);
  vm->status = vm->primitives[id](vm);
  if (!IsError(vm->status)) VMStackPush(vm->status.data.v, vm);
  vm->pc += LEBSize(id) + 1;
  return vm->status;
}

void VMTrace(VM *vm, char *src)
{
  TraceInst(vm);
  PrintStack(vm);
  fprintf(stderr, "\n");
}

static void TraceInst(VM *vm)
{
  u32 index = vm->pc;
  u32 i, len;
  len = DisassembleInst(vm->program->code, &index);
  if (len >= 20) {
    fprintf(stderr, "\n");
    len = 0;
  }
  for (i = 0; i < 20 - len; i++) fprintf(stderr, " ");
}

static u32 PrintStack(VM *vm)
{
  u32 i, printed = 0, max = 20;
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
