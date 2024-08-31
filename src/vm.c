#include "vm.h"
#include "env.h"
#include "result.h"
#include "leb.h"
#include "primitives.h"
#include "univ/file.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/symbol.h"
#include "univ/vec.h"
#include <SDL2/SDL.h>

typedef Result (*OpFn)(VM *vm);

static void TraceInst(VM *vm);
static u32 PrintStack(VM *vm);
static PrimFn *InitPrimitives(void);

typedef struct {
  char *filename;
  u32 pos;
} StackTrace;

static StackTrace *BuildStackTrace(VM *vm);

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
static Result OpLink(VM *vm);
static Result OpUnlink(VM *vm);
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
  [opLink]    = OpLink,
  [opUnlink]  = OpUnlink,
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

void UnwindVM(VM *vm)
{
  vm->pc = RawInt(vm->stack[vm->link-1]);
  vm->link = RawInt(vm->stack[vm->link]);
}

Result RuntimeError(char *message, struct VM *vm)
{
  char *file = GetSourceFile(vm->pc, &vm->program->srcmap);
  u32 pos = GetSourcePos(vm->pc, &vm->program->srcmap);
  Error *error = NewError(NewString("Runtime error: ^"), file, pos, 0);
  error->message = FormatString(error->message, message);
  error->data = BuildStackTrace(vm);
  return Err(error);
}

void InitVM(VM *vm, Program *program)
{
  vm->status = Ok(0);
  vm->pc = 0;
  vm->env = 0;
  vm->stack = 0;
  vm->program = program;
  if (program) {
    char *names = program->strings;
    u32 len = VecCount(program->strings);
    char *end = names + len;
    while (names < end) {
      len = strlen(names);
      SymbolFrom(names, len);
      names += len + 1;
    }
  }
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
  InitMem(256);

  if (program->trace) {
    u32 num_width = NumDigits(VecCount(program->code), 10);
    u32 i;
    for (i = 0; i < num_width; i++) fprintf(stderr, "─");
    fprintf(stderr, "┬─inst─────────stack───────────────\n");
    while (!VMDone(&vm)) {
      VMTrace(&vm, 0);
      VMStep(&vm);
    }
  } else {
    while (!VMDone(&vm)) VMStep(&vm);
  }

  DestroyVM(&vm);
  return vm.status;
}

#define StackRef(i, vm) (vm)->stack[VecCount((vm)->stack) - 1 - (i)]

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
  if (vm->program->trace) fprintf(stderr, "GARBAGE DAY!!!\n");
  VMStackPush(vm->env, vm);
  VMStackPush(vm->mod, vm);
  CollectGarbage(vm->stack);
  vm->mod = VMStackPop(vm);
  vm->env = VMStackPop(vm);
  if (MemSize() > MemCapacity()/2 + MemCapacity()/4) {
    SizeMem(2*MemSize());
  } else if (MemSize() < MemCapacity()/4) {
    SizeMem(Max(256, MemSize()/2));
  }
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

#define UnaryOp(op, a)    VMStackPush(IntVal(op RawVal(a)), vm)
#define BinOp(a, op, b)   VMStackPush(IntVal(RawInt(a) op RawInt(b)), vm)
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

static Result OpLink(VM *vm)
{
  VMStackPush(IntVal(vm->link), vm);
  vm->link = VecCount(vm->stack) - 1;
  vm->pc++;
  return vm->status;
}

static Result OpUnlink(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsInt(a)) return RuntimeError("Link type error", vm);
  vm->link = RawInt(a);
  vm->pc++;
  return vm->status;
}

static Result OpAdd(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be added", vm);
  BinOp(a, +, b);
  vm->pc++;
  return vm->status;
}

static Result OpSub(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be subtracted", vm);
  BinOp(a, -, b);
  vm->pc++;
  return vm->status;
}

static Result OpMul(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be multiplied", vm);
  BinOp(a, *, b);
  vm->pc++;
  return vm->status;
}

static Result OpDiv(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be divided", vm);
  if (RawVal(b) == 0) return RuntimeError("Divide by zero", vm);
  BinOp(a, /, b);
  vm->pc++;
  return vm->status;
}

static Result OpRem(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be remaindered", vm);
  if (RawVal(b) == 0) return RuntimeError("Divide by zero", vm);
  BinOp(a, %, b);
  vm->pc++;
  return vm->status;
}

static Result OpAnd(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be and-ed", vm);
  BinOp(a, &, b);
  vm->pc++;
  return vm->status;
}

static Result OpOr(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be or-ed", vm);
  BinOp(a, |, b);
  vm->pc++;
  return vm->status;
}

static Result OpComp(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsInt(a)) return RuntimeError("Only integers can be ", vm);
  UnaryOp(~, a);
  vm->pc++;
  return vm->status;
}

static Result OpLt(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be compared", vm);
  BinOp(a, <, b);
  vm->pc++;
  return vm->status;
}

static Result OpGt(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) return RuntimeError("Only integers can be compared", vm);
  BinOp(a, >, b);
  vm->pc++;
  return vm->status;
}

static Result OpEq(VM *vm)
{
  val a, b;
  TwoArgs(a, b);
  VMStackPush(IntVal(ValEq(a, b)), vm);
  vm->pc++;
  return vm->status;
}

static Result OpNeg(VM *vm)
{
  val a;
  OneArg(a);
  if (!IsInt(a)) return RuntimeError("Only integers can be negated", vm);
  UnaryOp(-, a);
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
  BinOp(a, <<, b);
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
  u32 x;
  OneArg(a);
  if (!IsInt(a)) return RuntimeError("Only symbols can become strings", vm);
  name = SymbolName(RawVal(a));
  if (!name) return RuntimeError("Only symbols can become strings", vm);
  len = strlen(name);
  x = BinSpace(len) + 1;
  MaybeGC(x, vm);
  VMStackPush(BinaryFrom(name, len), vm);
  vm->pc++;
  return vm->status;
}

static Result OpJoin(VM *vm)
{
  val a, b;
  CheckStack(vm, 2);
  b = StackRef(0, vm);
  a = StackRef(1, vm);
  if (ValType(a) != ValType(b)) return RuntimeError("Only values of the same type can be joined", vm);
  if (IsPair(a)) {
    if (!a) {
      VMStackPop(vm);
      VMStackPop(vm);
      VMStackPush(b, vm);
    } else if (!b) {
      VMStackPop(vm);
    } else {
      MaybeGC(2*(ListLength(a) + ListLength(b)), vm);
      TwoArgs(a, b);
      VMStackPush(ListJoin(a, b), vm);
    }
  } else if (IsTuple(a)) {
    if (TupleLength(a) == 0) {
      VMStackPop(vm);
      VMStackPop(vm);
      VMStackPush(b, vm);
    } else if (TupleLength(b) == 0) {
      VMStackPop(vm);
    } else {
      MaybeGC(1 + TupleLength(a) + TupleLength(b), vm);
      TwoArgs(a, b);
      VMStackPush(TupleJoin(a, b), vm);
    }
  } else if (IsBinary(a)) {
    if (BinaryLength(a) == 0) {
      VMStackPop(vm);
      VMStackPop(vm);
      VMStackPush(b, vm);
    } else if (BinaryLength(b) == 0) {
      VMStackPop(vm);
    } else {
      MaybeGC(1 + BinSpace(BinaryLength(a)) + BinSpace(BinaryLength(b)), vm);
      TwoArgs(a, b);
      VMStackPush(BinaryJoin(a, b), vm);
    }
  } else {
    return RuntimeError("Only lists, tuples, and binaries can be joined", vm);
  }
  vm->pc++;
  return vm->status;
}

static Result OpSlice(VM *vm)
{
  val a, b, c;
  TwoArgs(b, c);
  CheckStack(vm, 1);
  a = StackRef(0, vm);
  if (!IsInt(b)) return RuntimeError("Only integers can be slice indexes", vm);
  if (RawInt(b) < 0) return RuntimeError("Out of bounds", vm);
  if (c && !IsInt(c)) return RuntimeError("Only integers can be slice indexes", vm);

  if (IsPair(a)) {
    val list;
    u32 end = ListLength(a);
    if (c) {
      u32 len;
      if (RawInt(c) < 0 || RawInt(c) > (i32)end) return RuntimeError("Out of bounds", vm);
      len = RawInt(c) - RawInt(b);
      if (RawInt(c) < (i32)end) MaybeGC(4*len, vm);
      end = RawInt(c);
    }
    a = VMStackPop(vm);
    list = ListSlice(a, RawVal(b), end);
    VMStackPush(list, vm);
  } else if (IsTuple(a)) {
    u32 len;
    if (!c) c = IntVal(TupleLength(a));
    if (RawInt(c) < 0 || RawInt(c) > (i32)TupleLength(a)) return RuntimeError("Out of bounds", vm);
    len = RawVal(c) - RawVal(b);
    MaybeGC(len+1, vm);
    a = VMStackPop(vm);
    VMStackPush(TupleSlice(a, RawVal(b), RawVal(c)), vm);
  } else if (IsBinary(a)) {
    u32 len;
    if (!c) c = IntVal(BinaryLength(a));
    if (RawInt(c) < 0 || RawInt(c) > (i32)BinaryLength(a)) return RuntimeError("Out of bounds", vm);
    len = RawVal(c) - RawVal(b);
    MaybeGC(BinSpace(len)+1, vm);
    a = VMStackPop(vm);
    VMStackPush(BinarySlice(a, RawVal(b), RawVal(c)), vm);
  } else {
    return RuntimeError("Only lists, tuples, and binaries can be sliced", vm);
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
  if (VecCount(vm->stack) > max) printed += fprintf(stderr, "... [%d]", VecCount(vm->stack) - max);
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

static StackTrace *BuildStackTrace(VM *vm)
{
  u32 link = vm->link;
  StackTrace *trace = 0;
  StackTrace item;

  item.filename = GetSourceFile(vm->pc, &vm->program->srcmap);
  if (item.filename) {
    item.pos = GetSourcePos(vm->pc, &vm->program->srcmap);
  } else {
    item.pos = vm->pc;
  }
  VecPush(trace, item);

  while (link > 0) {
    u32 code_pos = RawInt(vm->stack[link-1]);
    item.filename = GetSourceFile(code_pos, &vm->program->srcmap);
    if (item.filename) {
      item.pos = GetSourcePos(code_pos, &vm->program->srcmap);
    } else {
      item.pos = code_pos;
    }
    VecPush(trace, item);
    link = RawInt(vm->stack[link]);
  }
  return trace;
}

void FreeStackTrace(Result result)
{
  Error *error = result.data.p;
  StackTrace *trace = error->data;
  FreeVec(trace);
}

void PrintStackTrace(Result result)
{
  Error *error = result.data.p;
  StackTrace *trace = error->data;
  u32 i;
  u32 colwidth = 0;

  for (i = 0; i < VecCount(trace); i++) {
    if (trace[i].filename) {
      char *text = ReadFile(trace[i].filename);
      if (text) {
        u32 line_num = LineNum(text, trace[i].pos);
        u32 col = ColNum(text, trace[i].pos);
        u32 len = snprintf(0, 0, "  %s:%d:%d: ", trace[i].filename, line_num+1, col+1);
        free(text);
        colwidth = Max(colwidth, len);
      }
    }
  }

  fprintf(stderr, "Stacktrace:\n");
  for (i = 0; i < VecCount(trace); i++) {
    if (trace[i].filename) {
      char *text = ReadFile(trace[i].filename);
      if (text) {
        u32 line = LineNum(text, trace[i].pos);
        u32 col = ColNum(text, trace[i].pos);
        u32 j, printed;
        printed = fprintf(stderr, "  %s:%d:%d: ", trace[i].filename, line+1, col+1);
        for (j = 0; j < colwidth - printed; j++) fprintf(stderr, " ");
        PrintSourceLine(text, trace[i].pos);
        free(text);
      } else {
        fprintf(stderr, "%s@%d\n", trace[i].filename, trace[i].pos);
      }
    } else {
      fprintf(stderr, "  (system)@%d\n", trace[i].pos);
    }
  }
}
