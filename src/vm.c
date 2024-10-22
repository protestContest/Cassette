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
#include <SDL2/SDL.h>

extern Mem mem;

typedef Result (*OpFn)(VM *vm);

static void TraceInst(VM *vm);
static u32 PrintStack(VM *vm, u32 max);
static PrimFn *InitPrimitives(void);

typedef struct {
  char *filename;
  u32 pos;
} StackTrace;

static StackTrace *BuildStackTrace(VM *vm);

static void PrintEnv(u32 env)
{
  while (env) {
    u32 frame;
    assert(IsPair(env));
    frame = Head(env);
    fprintf(stderr, "%s[%s] -> ", MemValStr(env), MemValStr(frame));
    env = Tail(env);
  }
  fprintf(stderr, "nil\n");
}

static Result OpNoop(VM *vm);
static Result OpHalt(VM *vm);
static Result OpPanic(VM *vm);
static Result OpConst(VM *vm);
static Result OpJump(VM *vm);
static Result OpBranch(VM *vm);
static Result OpPos(VM *vm);
static Result OpGoto(VM *vm);
static Result OpPush(VM *vm);
static Result OpPull(VM *vm);
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
  u32 i;
  vm->status = Ok(0);
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

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
}

void DestroyVM(VM *vm)
{
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
  char *start, *end;
  if (!src) {
    fprintf(stderr, "(system)");
    return;
  }
  start = src+index;
  end = LineEnd(index, src);
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

#define OneArg(a) \
  CheckStack(vm, 1);\
  a = StackPop()

#define TwoArgs(a, b) \
  CheckStack(vm, 2);\
  b = StackPop();\
  a = StackPop()

#define ThreeArgs(a, b, c) \
  CheckStack(vm, 3);\
  c = StackPop();\
  b = StackPop();\
  a = StackPop()

#define UnaryOp(op, a)    StackPush(IntVal(op RawVal(a)))
#define BinOp(a, op, b)   StackPush(IntVal(RawInt(a) op RawInt(b)))
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

static Result OpPanic(VM *vm)
{
  Result result;
  char *msg = 0;
  if (StackSize() > 0) {
    u32 msgVal = StackPop();
    if (IsBinary(msgVal)) {
      msg = StringFrom(BinaryData(msgVal), ObjLength(msgVal));
    }
  }
  if (!msg) msg = NewString("Panic!");
  result = RuntimeError(msg, vm);
  free(msg);
  return result;
}

static Result OpConst(VM *vm)
{
  u32 value = ReadLEB(++vm->pc, vm->program->code);
  vm->pc += LEBSize(value);
  MaybeGC(1, vm);
  StackPush(value);
  return vm->status;
}

static Result OpPos(VM *vm)
{
  i32 n = ReadLEB(++vm->pc, vm->program->code);
  vm->pc += LEBSize(n);
  MaybeGC(1, vm);
  StackPush(IntVal((i32)vm->pc + n));
  return vm->status;
}

static Result OpGoto(VM *vm)
{
  u32 a;
  OneArg(a);
  if (!IsInt(a)) return RuntimeError("Invalid address", vm);
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
  u32 a;
  i32 n = ReadLEB(++vm->pc, vm->program->code);
  vm->pc += LEBSize(n);
  OneArg(a);
  if (RawVal(a)) {
    CheckBounds((i32)vm->pc + n);
    vm->pc += n;
  }
  return vm->status;
}

static Result OpPush(VM *vm)
{
  i32 n = ReadLEB(++vm->pc, vm->program->code);
  MaybeGC(1, vm);
  StackPush(vm->regs[n]);
  vm->pc++;
  return vm->status;
}

static Result OpPull(VM *vm)
{
  i32 n = ReadLEB(++vm->pc, vm->program->code);
  CheckStack(vm, 1);
  vm->regs[n] = StackPop();
  vm->pc++;
  return vm->status;
}

static Result OpLink(VM *vm)
{
  MaybeGC(1, vm);
  StackPush(IntVal(vm->link));
  vm->link = StackSize();
  vm->pc++;
  return vm->status;
}

static Result OpUnlink(VM *vm)
{
  u32 a;
  OneArg(a);
  if (!IsInt(a)) return RuntimeError("Invalid stack link", vm);
  vm->link = RawInt(a);
  vm->pc++;
  return vm->status;
}

static Result OpAdd(VM *vm)
{
  u32 a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) {
    return RuntimeError("Only integers can be added", vm);
  }
  BinOp(a, +, b);
  vm->pc++;
  return vm->status;
}

static Result OpSub(VM *vm)
{
  u32 a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) {
    return RuntimeError("Only integers can be subtracted", vm);
  }
  BinOp(a, -, b);
  vm->pc++;
  return vm->status;
}

static Result OpMul(VM *vm)
{
  u32 a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) {
    return RuntimeError("Only integers can be multiplied", vm);
  }
  BinOp(a, *, b);
  vm->pc++;
  return vm->status;
}

static Result OpDiv(VM *vm)
{
  u32 a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) {
    return RuntimeError("Only integers can be divided", vm);
  }
  if (RawVal(b) == 0) return RuntimeError("Divide by zero", vm);
  BinOp(a, /, b);
  vm->pc++;
  return vm->status;
}

static Result OpRem(VM *vm)
{
  u32 a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) {
    return RuntimeError("Only integers can be remaindered", vm);
  }
  if (RawVal(b) == 0) return RuntimeError("Divide by zero", vm);
  BinOp(a, %, b);
  vm->pc++;
  return vm->status;
}

static Result OpAnd(VM *vm)
{
  u32 a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) {
    return RuntimeError("Only integers can be and-ed", vm);
  }
  BinOp(a, &, b);
  vm->pc++;
  return vm->status;
}

static Result OpOr(VM *vm)
{
  u32 a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) {
    return RuntimeError("Only integers can be or-ed", vm);
  }
  BinOp(a, |, b);
  vm->pc++;
  return vm->status;
}

static Result OpComp(VM *vm)
{
  u32 a;
  OneArg(a);
  if (!IsInt(a)) return RuntimeError("Only integers can be complemented", vm);
  UnaryOp(~, a);
  vm->pc++;
  return vm->status;
}

static Result OpLt(VM *vm)
{
  u32 a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) {
    return RuntimeError("Only integers can be compared", vm);
  }
  BinOp(a, <, b);
  vm->pc++;
  return vm->status;
}

static Result OpGt(VM *vm)
{
  u32 a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) {
    return RuntimeError("Only integers can be compared", vm);
  }
  BinOp(a, >, b);
  vm->pc++;
  return vm->status;
}

static Result OpEq(VM *vm)
{
  u32 a, b;
  TwoArgs(a, b);
  StackPush(IntVal(ValEq(a, b)));
  vm->pc++;
  return vm->status;
}

static Result OpNeg(VM *vm)
{
  u32 a;
  OneArg(a);
  if (!IsInt(a)) return RuntimeError("Only integers can be negated", vm);
  UnaryOp(-, a);
  vm->pc++;
  return vm->status;
}

static Result OpNot(VM *vm)
{
  u32 a;
  OneArg(a);
  StackPush(IntVal(RawVal(a) == 0));
  vm->pc++;
  return vm->status;
}

static Result OpShift(VM *vm)
{
  u32 a, b;
  TwoArgs(a, b);
  if (!IsInt(a) || !IsInt(b)) {
    return RuntimeError("Only integers can be shifted", vm);
  }
  BinOp(a, <<, b);
  vm->pc++;
  return vm->status;
}

static Result OpDup(VM *vm)
{
  u32 a;
  MaybeGC(1, vm);
  OneArg(a);
  StackPush(a);
  StackPush(a);
  vm->pc++;
  return vm->status;
}

static Result OpDrop(VM *vm)
{
  CheckStack(vm, 1);
  StackPop();
  vm->pc++;
  return vm->status;
}

static Result OpSwap(VM *vm)
{
  u32 a, b;
  TwoArgs(a, b);
  StackPush(b);
  StackPush(a);
  vm->pc++;
  return vm->status;
}

static Result OpOver(VM *vm)
{
  u32 a, b;
  MaybeGC(1, vm);
  TwoArgs(a, b);
  StackPush(a);
  StackPush(b);
  StackPush(a);
  vm->pc++;
  return vm->status;
}

static Result OpRot(VM *vm)
{
  u32 a, b, c;
  ThreeArgs(a, b, c);
  StackPush(b);
  StackPush(c);
  StackPush(a);
  vm->pc++;
  return vm->status;
}

static Result OpPick(VM *vm)
{
  u32 n = ReadLEB(++vm->pc, vm->program->code);
  u32 v;
  vm->pc += LEBSize(n);
  CheckStack(vm, n);
  if (n < 0) return RuntimeError("Invalid stack index", vm);

  MaybeGC(1, vm);
  v = StackPeek(n);
  StackPush(v);
  return vm->status;
}

static Result OpPair(VM *vm)
{
  u32 a, b;
  MaybeGC(1, vm);
  TwoArgs(a, b);
  StackPush(Pair(b, a));
  vm->pc++;
  return vm->status;
}

static Result OpHead(VM *vm)
{
  u32 a;
  OneArg(a);
  if (!IsPair(a)) return RuntimeError("Only pairs have heads", vm);
  StackPush(Head(a));
  vm->pc++;
  return vm->status;
}

static Result OpTail(VM *vm)
{
  u32 a;
  OneArg(a);
  if (!IsPair(a)) return RuntimeError("Only pairs have tails", vm);
  StackPush(Tail(a));
  vm->pc++;
  return vm->status;
}

static Result OpTuple(VM *vm)
{
  u32 count = ReadLEB(++vm->pc, vm->program->code);
  vm->pc += LEBSize(count);
  MaybeGC(count+1, vm);
  StackPush(Tuple(count));
  return vm->status;
}

static Result OpLen(VM *vm)
{
  u32 a;
  OneArg(a);
  if (!IsTuple(a) || !IsBinary(a)) {
    return RuntimeError("Only tuples and binaries have lengths", vm);
  }
  StackPush(IntVal(ObjLength(a)));
  vm->pc++;
  return vm->status;
}

static Result OpGet(VM *vm)
{
  u32 a, b;
  TwoArgs(a, b);
  if (!IsInt(b)) return RuntimeError("Only integers can be indexes", vm);
  if (RawInt(b) < 0 || RawInt(b) >= (i32)ObjLength(a)) {
    return RuntimeError("Out of bounds", vm);
  }
  if (IsTuple(a)) {
    StackPush(TupleGet(a, RawInt(b)));
  } else if (IsBinary(a)) {
    StackPush(IntVal(BinaryGet(a, RawInt(b))));
  } else {
    return RuntimeError("Only tuples and binaries can be accessed", vm);
  }
  vm->pc++;
  return vm->status;
}

static Result OpSet(VM *vm)
{
  u32 a, b, c;
  ThreeArgs(a, b, c);
  if (!IsInt(b)) return RuntimeError("Only integers can be indexes", vm);
  if (RawInt(b) < 0) return RuntimeError("Out of bounds", vm);
  if (IsTuple(a)) {
    TupleSet(a, RawVal(b), c);
  } else if (IsBinary(a)) {
    BinarySet(a, RawVal(b), c);
  } else {
    return RuntimeError("Only tuples and binaries can be accessed", vm);
  }
  StackPush(a);
  vm->pc++;
  return vm->status;
}

static Result OpStr(VM *vm)
{
  char *name;
  u32 len;
  u32 a;
  u32 x;
  OneArg(a);
  if (!IsInt(a)) return RuntimeError("Only symbols can become strings", vm);
  name = SymbolName(RawVal(a));
  if (!name) return RuntimeError("Only symbols can become strings", vm);
  len = strlen(name);
  x = BinSpace(len) + 1;
  MaybeGC(x, vm);
  StackPush(BinaryFrom(name, len));
  vm->pc++;
  return vm->status;
}

static Result OpJoin(VM *vm)
{
  u32 a, b;
  CheckStack(vm, 2);
  b = StackPeek(0);
  a = StackPeek(1);
  if (ValType(a) != ValType(b)) {
    return RuntimeError("Only values of the same type can be joined", vm);
  }
  if (!IsTuple(a) && !IsBinary(a)) {
    return RuntimeError("Only tuples and binaries can be joined", vm);
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
      TwoArgs(a, b);
      StackPush(TupleJoin(a, b));
    } else if (IsBinary(a)) {
      MaybeGC(1 + BinSpace(ObjLength(a) + ObjLength(b)), vm);
      TwoArgs(a, b);
      StackPush(BinaryJoin(a, b));
    }
  }
  vm->pc++;
  return vm->status;
}

static Result OpSlice(VM *vm)
{
  u32 a, b, c, len;
  TwoArgs(b, c);
  CheckStack(vm, 1);
  a = StackPeek(0);
  if (!IsInt(b) || !IsInt(c)) {
    return RuntimeError("Only integers can be slice indexes", vm);
  }
  if (RawInt(b) < 0 || RawInt(c) < RawInt(b) || RawInt(c) > (i32)ObjLength(a)) {
    return RuntimeError("Out of bounds", vm);
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
    return RuntimeError("Only tuples and binaries can be sliced", vm);
  }
  vm->pc++;
  return vm->status;
}

static Result OpTrap(VM *vm)
{
  u32 id = ReadLEB(vm->pc+1, vm->program->code);
  vm->status = vm->primitives[id](vm);
  if (!IsError(vm->status)) StackPush(vm->status.data.v);
  vm->pc += LEBSize(id) + 1;
  return vm->status;
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
    u32 code_pos = RawInt(StackPeek(StackSize() - link - 1));
    item.filename = GetSourceFile(code_pos, &vm->program->srcmap);
    if (item.filename) {
      item.pos = GetSourcePos(code_pos, &vm->program->srcmap);
    } else {
      item.pos = code_pos;
    }
    VecPush(trace, item);
    link = RawInt(StackPeek(StackSize() - link));
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
        u32 len = snprintf(0, 0, "  %s:%d:%d: ",
            trace[i].filename, line_num+1, col+1);
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
        printed = fprintf(stderr, "  %s:%d:%d: ",
            trace[i].filename, line+1, col+1);
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
