#include "vm.h"
#include "leb.h"
#include "mem.h"
#include "ops.h"
#include "primitives.h"
#include "univ/file.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/symbol.h"
#include "univ/vec.h"

#ifdef SDL
#include <SDL2/SDL.h>
#endif

typedef void (*OpFn)(VM *vm);

static void TraceInst(VM *vm);
static u32 PrintStack(VM *vm, u32 max);
static PrimFn *InitPrimitives(void);
static StackTrace *BuildStackTrace(VM *vm);

static void OpNoop(VM *vm);
static void OpHalt(VM *vm);
static void OpPanic(VM *vm);
static void OpConst(VM *vm);
static void OpJump(VM *vm);
static void OpBranch(VM *vm);
static void OpPos(VM *vm);
static void OpGoto(VM *vm);
static void OpPush(VM *vm);
static void OpPull(VM *vm);
static void OpGetMod(VM *vm);
static void OpSetMod(VM *vm);
static void OpLink(VM *vm);
static void OpUnlink(VM *vm);
static void OpAdd(VM *vm);
static void OpSub(VM *vm);
static void OpMul(VM *vm);
static void OpDiv(VM *vm);
static void OpRem(VM *vm);
static void OpAnd(VM *vm);
static void OpOr(VM *vm);
static void OpComp(VM *vm);
static void OpLt(VM *vm);
static void OpGt(VM *vm);
static void OpEq(VM *vm);
static void OpNeg(VM *vm);
static void OpNot(VM *vm);
static void OpShift(VM *vm);
static void OpDup(VM *vm);
static void OpDrop(VM *vm);
static void OpSwap(VM *vm);
static void OpOver(VM *vm);
static void OpRot(VM *vm);
static void OpPick(VM *vm);
static void OpPair(VM *vm);
static void OpHead(VM *vm);
static void OpTail(VM *vm);
static void OpTuple(VM *vm);
static void OpLen(VM *vm);
static void OpGet(VM *vm);
static void OpSet(VM *vm);
static void OpStr(VM *vm);
static void OpJoin(VM *vm);
static void OpSlice(VM *vm);
static void OpTrap(VM *vm);

static OpFn ops[] = {
  [opNoop]    = OpNoop,
  [opHalt]    = OpHalt,
  [opPanic]   = OpPanic,
  [opConst]   = OpConst,
  [opJump]    = OpJump,
  [opBranch]  = OpBranch,
  [opPos]     = OpPos,
  [opGoto]    = OpGoto,
  [opPush]    = OpPush,
  [opPull]    = OpPull,
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

#define VMDone(vm) ((vm)->error || (vm)->pc >= VecCount((vm)->program->code))
#define BinOp(a, op, b)   StackPush(IntVal(RawInt(a) op RawInt(b)))

Error *RuntimeError(char *message, struct VM *vm)
{
  char *file = GetSourceFile(vm->pc, &vm->program->srcmap);
  u32 pos = GetSourcePos(vm->pc, &vm->program->srcmap);
  Error *error = NewError(NewString("Runtime error: ^"), file, pos, 0);
  error->message = FormatString(error->message, message);
  error->data = BuildStackTrace(vm);
  return error;
}

void InitVM(VM *vm, Program *program)
{
  u32 i;
  vm->error = 0;
  vm->pc = 0;
  for (i = 0; i < ArrayCount(vm->regs); i++) vm->regs[i] = 0;
  vm->link = 0;
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

#ifdef SDL
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
#endif
}

void DestroyVM(VM *vm)
{
  FreeVec(vm->refs);
  free(vm->primitives);
#ifdef SDL
  SDL_Quit();
#endif
}

void VMStep(VM *vm)
{
  ops[vm->program->code[vm->pc]](vm);
}

Error *VMRun(Program *program)
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
  return vm.error;
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
  if (MemFree() < size) {
    u32 needed = MemCapacity() + size - MemFree();
    SizeMem(Max(2*MemCapacity(), needed));
  }
}

void RunGC(VM *vm)
{
  if (vm->program->trace) fprintf(stderr, "GARBAGE DAY!!!\n");
  CollectGarbage(vm->regs, ArrayCount(vm->regs));
  if (MemFree() < MemCapacity()/4) {
    SizeMem(2*MemCapacity());
  } else if (MemFree() > MemCapacity()/4 + MemCapacity()/2) {
    SizeMem(Max(256, MemCapacity()/2));
  }
}

static void OpNoop(VM *vm)
{
  vm->pc++;
}

static void OpHalt(VM *vm)
{
  vm->pc = VecCount(vm->program->code);
}

static void OpPanic(VM *vm)
{
  char *msg = 0;
  if (StackSize() > 0) {
    u32 msgVal = StackPop();
    if (IsBinary(msgVal)) {
      msg = StringFrom(BinaryData(msgVal), ObjLength(msgVal));
    }
  }
  if (!msg) msg = NewString("Panic!");
  RuntimeError(msg, vm);
  free(msg);
}

static void OpConst(VM *vm)
{
  u32 value = ReadLEB(++vm->pc, vm->program->code);
  vm->pc += LEBSize(value);
  MaybeGC(1, vm);
  StackPush(value);
}

static void OpPos(VM *vm)
{
  i32 n = ReadLEB(++vm->pc, vm->program->code);
  vm->pc += LEBSize(n);
  MaybeGC(1, vm);
  StackPush(IntVal((i32)vm->pc + n));
}

static void OpGoto(VM *vm)
{
  u32 a;
  if (StackSize() < 1) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  if (!IsInt(a)) {
    vm->error = RuntimeError("Invalid address", vm);
    return;
  }
  if (RawInt(a) < 0 || RawInt(a) > (i32)VecCount(vm->program->code)) {
    vm->error = RuntimeError("Out of bounds", vm);
    return;
  }
  vm->pc = RawVal(a);
}

static void OpJump(VM *vm)
{
  i32 n = ReadLEB(++vm->pc, vm->program->code);
  vm->pc += LEBSize(n);
  if (vm->pc + n < 0 || vm->pc + n > VecCount(vm->program->code)) {
    vm->error = RuntimeError("Out of bounds", vm);
    return;
  }
  vm->pc += n;
}

static void OpBranch(VM *vm)
{
  u32 a;
  i32 n = ReadLEB(++vm->pc, vm->program->code);
  vm->pc += LEBSize(n);
  if (StackSize() < 1) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }

  a = StackPop();
  if (RawVal(a)) {
    if (vm->pc + n < 0 || vm->pc + n > VecCount(vm->program->code)) {
      vm->error = RuntimeError("Out of bounds", vm);
      return;
    }
    vm->pc += n;
  }
}

static void OpPush(VM *vm)
{
  i32 n = ReadLEB(++vm->pc, vm->program->code);
  MaybeGC(1, vm);
  StackPush(vm->regs[n]);
  vm->pc++;
}

static void OpPull(VM *vm)
{
  i32 n = ReadLEB(++vm->pc, vm->program->code);
  if (StackSize() < 1) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  vm->regs[n] = StackPop();
  vm->pc++;
}

static void OpLink(VM *vm)
{
  MaybeGC(1, vm);
  StackPush(IntVal(vm->link));
  vm->link = StackSize();
  vm->pc++;
}

static void OpUnlink(VM *vm)
{
  u32 a;
  if (StackSize() < 1) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  if (!IsInt(a)) {
    vm->error = RuntimeError("Invalid stack link", vm);
    return;
  }
  vm->link = RawInt(a);
  vm->pc++;
}

static void OpAdd(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    vm->error = RuntimeError("Only integers can be added", vm);
    return;
  }
  BinOp(a, +, b);
  vm->pc++;
}

static void OpSub(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    vm->error = RuntimeError("Only integers can be subtracted", vm);
    return;
  }
  BinOp(a, -, b);
  vm->pc++;
}

static void OpMul(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    vm->error = RuntimeError("Only integers can be multiplied", vm);
    return;
  }
  BinOp(a, *, b);
  vm->pc++;
}

static void OpDiv(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    vm->error = RuntimeError("Only integers can be divided", vm);
    return;
  }
  if (RawVal(b) == 0) {
    vm->error = RuntimeError("Divide by zero", vm);
    return;
  }
  BinOp(a, /, b);
  vm->pc++;
}

static void OpRem(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    vm->error = RuntimeError("Only integers can be remaindered", vm);
    return;
  }
  if (RawVal(b) == 0) {
    vm->error = RuntimeError("Divide by zero", vm);
    return;
  }
  BinOp(a, %, b);
  vm->pc++;
}

static void OpAnd(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    vm->error = RuntimeError("Only integers can be and-ed", vm);
    return;
  }
  BinOp(a, &, b);
  vm->pc++;
}

static void OpOr(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    vm->error = RuntimeError("Only integers can be or-ed", vm);
    return;
  }
  BinOp(a, |, b);
  vm->pc++;
}

static void OpComp(VM *vm)
{
  u32 a;
  if (StackSize() < 1) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  if (!IsInt(a)) {
    vm->error = RuntimeError("Only integers can be complemented", vm);
    return;
  }
  StackPush(IntVal(~RawVal(a)));
  vm->pc++;
}

static void OpLt(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    vm->error = RuntimeError("Only integers can be compared", vm);
    return;
  }
  BinOp(a, <, b);
  vm->pc++;
}

static void OpGt(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    vm->error = RuntimeError("Only integers can be compared", vm);
    return;
  }
  BinOp(a, >, b);
  vm->pc++;
}

static void OpEq(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  StackPush(IntVal(ValEq(a, b)));
  vm->pc++;
}

static void OpNeg(VM *vm)
{
  u32 a;
  if (StackSize() < 1) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  if (!IsInt(a)) {
    vm->error = RuntimeError("Only integers can be negated", vm);
    return;
  }
  StackPush(IntVal(-RawInt(a)));
  vm->pc++;
}

static void OpNot(VM *vm)
{
  u32 a;
  if (StackSize() < 1) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  StackPush(IntVal(RawVal(a) == 0));
  vm->pc++;
}

static void OpShift(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    vm->error = RuntimeError("Only integers can be shifted", vm);
    return;
  }
  BinOp(a, <<, b);
  vm->pc++;
}

static void OpDup(VM *vm)
{
  u32 a;
  MaybeGC(1, vm);
  if (StackSize() < 1) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  StackPush(a);
  StackPush(a);
  vm->pc++;
}

static void OpDrop(VM *vm)
{
  if (StackSize() < 1) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  StackPop();
  vm->pc++;
}

static void OpSwap(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  StackPush(b);
  StackPush(a);
  vm->pc++;
}

static void OpOver(VM *vm)
{
  u32 a, b;
  MaybeGC(1, vm);
  if (StackSize() < 2) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  StackPush(a);
  StackPush(b);
  StackPush(a);
  vm->pc++;
}

static void OpRot(VM *vm)
{
  u32 a, b, c;
  if (StackSize() < 3) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  c = StackPop();
  b = StackPop();
  a = StackPop();
  StackPush(b);
  StackPush(c);
  StackPush(a);
  vm->pc++;
}

static void OpPick(VM *vm)
{
  u32 n = ReadLEB(++vm->pc, vm->program->code);
  u32 v;
  vm->pc += LEBSize(n);
  if (StackSize() < n) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  if (n < 0) {
    vm->error = RuntimeError("Invalid stack index", vm);
    return;
  }
  MaybeGC(1, vm);
  v = StackPeek(n);
  StackPush(v);
}

static void OpPair(VM *vm)
{
  u32 a, b;
  MaybeGC(1, vm);
  if (StackSize() < 2) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  StackPush(Pair(b, a));
  vm->pc++;
}

static void OpHead(VM *vm)
{
  u32 a;
  if (StackSize() < 1) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  if (!IsPair(a)) {
    vm->error = RuntimeError("Only pairs have heads", vm);
    return;
  }
  StackPush(Head(a));
  vm->pc++;
}

static void OpTail(VM *vm)
{
  u32 a;
  if (StackSize() < 1) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  if (!IsPair(a)) {
    vm->error = RuntimeError("Only pairs have tails", vm);
    return;
  }
  StackPush(Tail(a));
  vm->pc++;
}

static void OpTuple(VM *vm)
{
  u32 count = ReadLEB(++vm->pc, vm->program->code);
  vm->pc += LEBSize(count);
  MaybeGC(count+1, vm);
  StackPush(Tuple(count));
}

static void OpLen(VM *vm)
{
  u32 a;
  if (StackSize() < 1) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  if (!IsTuple(a) && !IsBinary(a)) {
    vm->error = RuntimeError("Only tuples and binaries have lengths", vm);
    return;
  }
  StackPush(IntVal(ObjLength(a)));
  vm->pc++;
}

static void OpGet(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(b)) {
    vm->error = RuntimeError("Only integers can be indexes", vm);
    return;
  }
  if (RawInt(b) < 0 || RawInt(b) >= (i32)ObjLength(a)) {
    vm->error = RuntimeError("Out of bounds", vm);
    return;
  }
  if (IsTuple(a)) {
    StackPush(TupleGet(a, RawInt(b)));
  } else if (IsBinary(a)) {
    StackPush(IntVal(BinaryGet(a, RawInt(b))));
  } else {
    vm->error = RuntimeError("Only tuples and binaries can be accessed", vm);
    return;
  }
  vm->pc++;
}

static void OpSet(VM *vm)
{
  u32 a, b, c;
  if (StackSize() < 3) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  c = StackPop();
  b = StackPop();
  a = StackPop();
  if (!IsInt(b)) {
    vm->error = RuntimeError("Only integers can be indexes", vm);
    return;
  }
  if (RawInt(b) < 0) {
    vm->error = RuntimeError("Out of bounds", vm);
    return;
  }
  if (IsTuple(a)) {
    TupleSet(a, RawVal(b), c);
  } else if (IsBinary(a)) {
    BinarySet(a, RawVal(b), c);
  } else {
    vm->error = RuntimeError("Only tuples and binaries can be accessed", vm);
    return;
  }
  StackPush(a);
  vm->pc++;
}

static void OpStr(VM *vm)
{
  char *name;
  u32 len;
  u32 a;
  u32 x;
  if (StackSize() < 1) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  if (!IsInt(a)) {
    vm->error = RuntimeError("Only symbols can become strings", vm);
    return;
  }
  name = SymbolName(RawVal(a));
  if (!name) {
    vm->error = RuntimeError("Only symbols can become strings", vm);
    return;
  }
  len = strlen(name);
  x = BinSpace(len) + 1;
  MaybeGC(x, vm);
  StackPush(BinaryFrom(name, len));
  vm->pc++;
}

static void OpJoin(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPeek(0);
  a = StackPeek(1);
  if (ValType(a) != ValType(b)) {
    vm->error = RuntimeError("Only values of the same type can be joined", vm);
    return;
  }
  if (!IsTuple(a) && !IsBinary(a)) {
    vm->error = RuntimeError("Only tuples and binaries can be joined", vm);
    return;
  }

  if (ObjLength(a) == 0) {
    StackPop();
    StackPop();
    StackPush(b);
  } else if (ObjLength(b) == 0) {
    StackPop();
  } else {
    if (IsTuple(a)) {
      MaybeGC(1 + ObjLength(a) + ObjLength(b), vm);
      if (StackSize() < 2) {
        vm->error = RuntimeError("Stack underflow", vm);
        return;
      }

      b = StackPop();
      a = StackPop();
      StackPush(TupleJoin(a, b));
    } else if (IsBinary(a)) {
      MaybeGC(1 + BinSpace(ObjLength(a) + ObjLength(b)), vm);
      if (StackSize() < 2) {
        vm->error = RuntimeError("Stack underflow", vm);
        return;
      }

      b = StackPop();
      a = StackPop();
      StackPush(BinaryJoin(a, b));
    }
  }
  vm->pc++;
}

static void OpSlice(VM *vm)
{
  u32 a, b, c, len;
  if (StackSize() < 3) {
    vm->error = RuntimeError("Stack underflow", vm);
    return;
  }
  c = StackPop();
  b = StackPop();
  a = StackPeek(0);
  if (!IsInt(b) || !IsInt(c)) {
    vm->error = RuntimeError("Only integers can be slice indexes", vm);
    return;
  }
  if (RawInt(b) < 0 || RawInt(c) < RawInt(b) || RawInt(c) > (i32)ObjLength(a)) {
    vm->error = RuntimeError("Out of bounds", vm);
    return;
  }
  len = RawVal(c) - RawVal(b);

  if (IsTuple(a)) {
    MaybeGC(len+1, vm);
    a = StackPop();
    StackPush(TupleSlice(a, RawVal(b), RawVal(c)));
  } else if (IsBinary(a)) {
    MaybeGC(BinSpace(len)+1, vm);
    a = StackPop();
    StackPush(BinarySlice(a, RawVal(b), RawVal(c)));
  } else {
    vm->error = RuntimeError("Only tuples and binaries can be sliced", vm);
    return;
  }
  vm->pc++;
}

static void OpTrap(VM *vm)
{
  u32 id = ReadLEB(vm->pc+1, vm->program->code);
  u32 value;
  MaybeGC(1, vm);
  value = vm->primitives[id](vm);
  if (!vm->error) {
    StackPush(value);
  }
  vm->pc += LEBSize(id) + 1;
}

void VMTrace(VM *vm, char *src)
{
  TraceInst(vm);
  PrintStack(vm, 20);
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

static u32 PrintStack(VM *vm, u32 max)
{
  u32 i, printed = 0;
  char *str;
  for (i = 0; i < max; i++) {
    if (i >= StackSize()) break;
    str = MemValStr(StackPeek(i));
    printed += fprintf(stderr, "%s ", str);
    free(str);
  }
  if (StackSize() > max) {
    printed += fprintf(stderr, "... [%d]", StackSize() - max);
  }
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

static StackTrace *NewStackTrace(StackTrace *next)
{
  StackTrace *trace = malloc(sizeof(StackTrace));
  trace->filename = 0;
  trace->pos = 0;
  trace->next = next;
  return trace;
}

static StackTrace *BuildStackTrace(VM *vm)
{
  u32 link = vm->link;
  StackTrace *trace = NewStackTrace(0);
  StackTrace *item;

  trace->filename = GetSourceFile(vm->pc, &vm->program->srcmap);
  if (trace->filename) {
    trace->pos = GetSourcePos(vm->pc, &vm->program->srcmap);
  } else {
    trace->pos = vm->pc;
  }

  while (link > 0) {
    u32 code_pos = RawInt(StackPeek(StackSize() - link - 1));
    item = NewStackTrace(trace);
    item->filename = GetSourceFile(code_pos, &vm->program->srcmap);
    if (item->filename) {
      item->pos = GetSourcePos(code_pos, &vm->program->srcmap);
    } else {
      item->pos = code_pos;
    }
    trace = item;
    link = RawInt(StackPeek(StackSize() - link));
  }
  return trace;
}

void FreeStackTrace(StackTrace *st)
{
  while (st) {
    StackTrace *next = st->next;
    free(st);
    st = next;
  }
}

void PrintStackTrace(StackTrace *st)
{
  u32 colwidth = 0;
  StackTrace *trace = st;

  while (trace) {
    if (trace->filename) {
      char *text = ReadFile(trace->filename);
      if (text) {
        u32 line_num = LineNum(text, trace->pos);
        u32 col = ColNum(text, trace->pos);
        u32 len = snprintf(0, 0, "  %s:%d:%d: ",
            trace->filename, line_num+1, col+1);
        free(text);
        colwidth = Max(colwidth, len);
      }
    }
    trace = trace->next;
  }

  trace = st;
  fprintf(stderr, "Stacktrace:\n");
  while (trace) {
    if (trace->filename) {
      char *text = ReadFile(trace->filename);
      if (text) {
        u32 line = LineNum(text, trace->pos);
        u32 col = ColNum(text, trace->pos);
        u32 j, printed;
        printed = fprintf(stderr, "  %s:%d:%d: ",
            trace->filename, line+1, col+1);
        for (j = 0; j < colwidth - printed; j++) fprintf(stderr, " ");
        PrintSourceLine(text, trace->pos);
        free(text);
      } else {
        fprintf(stderr, "%s@%d\n", trace->filename, trace->pos);
      }
    } else {
      fprintf(stderr, "  (system)@%d\n", trace->pos);
    }
    trace = trace->next;
  }
}
