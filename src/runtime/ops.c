#include "runtime/ops.h"
#include "runtime/mem.h"
#include "runtime/primitives.h"
#include "runtime/symbol.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/vec.h"

char *OpName(OpCode op)
{
  switch (op) {
  case opNoop:    return "noop";
  case opHalt:    return "halt";
  case opPanic:   return "panic";
  case opConst:   return "const";
  case opLookup:  return "lookup";
  case opDefine:  return "define";
  case opJump:    return "jump";
  case opBranch:  return "branch";
  case opPos:     return "pos";
  case opGoto:    return "goto";
  case opPush:    return "push";
  case opPull:    return "pull";
  case opLink:    return "link";
  case opUnlink:  return "unlink";
  case opAdd:     return "add";
  case opSub:     return "sub";
  case opMul:     return "mul";
  case opDiv:     return "div";
  case opRem:     return "rem";
  case opAnd:     return "and";
  case opOr:      return "or";
  case opComp:    return "comp";
  case opLt:      return "lt";
  case opGt:      return "gt";
  case opEq:      return "eq";
  case opNeg:     return "neg";
  case opNot:     return "not";
  case opShift:   return "shift";
  case opXor:     return "xor";
  case opDup:     return "dup";
  case opDrop:    return "drop";
  case opSwap:    return "swap";
  case opOver:    return "over";
  case opRot:     return "rot";
  case opPick:    return "pick";
  case opPair:    return "pair";
  case opHead:    return "head";
  case opTail:    return "tail";
  case opTuple:   return "tuple";
  case opLen:     return "len";
  case opGet:     return "get";
  case opSet:     return "set";
  case opStr:     return "str";
  case opJoin:    return "join";
  case opSlice:   return "slice";
  case opTrap:    return "trap";
  default:        return "???";
  }
}

u32 DisassembleInst(u8 *code, u32 *index)
{
  OpCode op = code[*index];
  u32 arg;
  u32 len = 0;
  char *arg_str;
  u32 num_width = NumDigits(VecCount(code), 10);

  len += fprintf(stderr, "%*d│ %s", num_width, *index, OpName(op)) - 2;
  (*index)++;

  switch (op) {
  case opConst:
    arg = ReadLEB(*index, code);
    arg_str = MemValStr(arg);
    len += fprintf(stderr, " %s", arg_str);
    free(arg_str);
    (*index) += LEBSize(arg);
    return len;
  case opLookup:
  case opDefine:
  case opTuple:
  case opBranch:
  case opJump:
  case opPos:
  case opPush:
  case opPull:
  case opPick:
  case opTrap:
    arg = ReadLEB(*index, code);
    len += fprintf(stderr, " %d", arg);
    (*index) += LEBSize(arg);
    return len;
  default:
    return len;
  }
}

void Disassemble(u8 *code)
{
  u32 end = VecCount(code);
  u32 index = 0;
  u32 num_width = NumDigits(VecCount(code), 10);
  u32 i;

  if (!code) {
    fprintf(stderr, "--Empty--\n");
    return;
  }

  for (i = 0; i < num_width; i++) fprintf(stderr, "─");
  fprintf(stderr, "┬─disassembly─────\n");
  while (index < end) {
    DisassembleInst(code, &index);
    fprintf(stderr, "\n");
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
    } else if (IsInt(msgVal)) {
      char *name = SymbolName(RawVal(msgVal));
      if (name) {
        msg = StringFrom(name, StrLen(name));
      }
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

static void OpLookup(VM *vm)
{
  u32 n = ReadLEB(++vm->pc, vm->program->code);
  u32 env;
  if (StackSize() < 1) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  env = StackPop();
  while (env) {
    u32 frame = Head(env);
    if (ObjLength(frame) > n) {
      StackPush(TupleGet(frame, n));
      break;
    }
    n -= ObjLength(frame);
    env = Tail(env);
  }
  if (!env) {
    RuntimeError("Undefined variable", vm);
    return;
  }
  vm->pc += LEBSize(n);
}

static void OpDefine(VM *vm)
{
  u32 n = ReadLEB(++vm->pc, vm->program->code);
  u32 env, value;
  if (StackSize() < 2) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  value = StackPop();
  env = StackPop();
  while (env) {
    u32 frame = Head(env);
    if (ObjLength(frame) > n) {
      TupleSet(frame, n, value);
      break;
    }
    n -= ObjLength(frame);
    env = Tail(env);
  }
  if (!env) {
    RuntimeError("Bad variable index", vm);
    return;
  }
  vm->pc += LEBSize(n);
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
    RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  if (!IsInt(a)) {
    RuntimeError("Invalid address", vm);
    return;
  }
  if (RawInt(a) < 0 || RawInt(a) > (i32)VecCount(vm->program->code)) {
    RuntimeError("Out of bounds", vm);
    return;
  }
  vm->pc = RawVal(a);
}

static void OpJump(VM *vm)
{
  i32 n = ReadLEB(++vm->pc, vm->program->code);
  vm->pc += LEBSize(n);
  if (vm->pc + n < 0 || vm->pc + n > VecCount(vm->program->code)) {
    RuntimeError("Out of bounds", vm);
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
    RuntimeError("Stack underflow", vm);
    return;
  }

  a = StackPop();
  if (RawVal(a)) {
    if (vm->pc + n < 0 || vm->pc + n > VecCount(vm->program->code)) {
      RuntimeError("Out of bounds", vm);
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
    RuntimeError("Stack underflow", vm);
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
    RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  if (!IsInt(a)) {
    RuntimeError("Invalid stack link", vm);
    return;
  }
  vm->link = RawInt(a);
  vm->pc++;
}

static void OpAdd(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    RuntimeError("Only integers can be added", vm);
    return;
  }
  StackPush(IntVal(RawInt(a) + RawInt(b)));
  vm->pc++;
}

static void OpSub(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    RuntimeError("Only integers can be subtracted", vm);
    return;
  }
  StackPush(IntVal(RawInt(a) - RawInt(b)));
  vm->pc++;
}

static void OpMul(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    RuntimeError("Only integers can be multiplied", vm);
    return;
  }
  StackPush(IntVal(RawInt(a) * RawInt(b)));
  vm->pc++;
}

static void OpDiv(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    RuntimeError("Only integers can be divided", vm);
    return;
  }
  if (RawVal(b) == 0) {
    RuntimeError("Divide by zero", vm);
    return;
  }
  StackPush(IntVal(RawInt(a) / RawInt(b)));
  vm->pc++;
}

static void OpRem(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    RuntimeError("Only integers can be remaindered", vm);
    return;
  }
  if (RawVal(b) == 0) {
    RuntimeError("Divide by zero", vm);
    return;
  }
  StackPush(IntVal(RawInt(a) % RawInt(b)));
  vm->pc++;
}

static void OpAnd(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    RuntimeError("Only integers can be and-ed", vm);
    return;
  }
  StackPush(IntVal(RawInt(a) & RawInt(b)));
  vm->pc++;
}

static void OpOr(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    RuntimeError("Only integers can be or-ed", vm);
    return;
  }
  StackPush(IntVal(RawInt(a) | RawInt(b)));
  vm->pc++;
}

static void OpComp(VM *vm)
{
  u32 a;
  if (StackSize() < 1) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  if (!IsInt(a)) {
    RuntimeError("Only integers can be complemented", vm);
    return;
  }
  StackPush(IntVal(~RawVal(a)));
  vm->pc++;
}

static void OpLt(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    RuntimeError("Only integers can be compared", vm);
    return;
  }
  StackPush(IntVal(RawInt(a) < RawInt(b)));
  vm->pc++;
}

static void OpGt(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    RuntimeError("Only integers can be compared", vm);
    return;
  }
  StackPush(IntVal(RawInt(a) > RawInt(b)));
  vm->pc++;
}

static void OpEq(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    RuntimeError("Stack underflow", vm);
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
    RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  if (!IsInt(a)) {
    RuntimeError("Only integers can be negated", vm);
    return;
  }
  StackPush(IntVal(-RawInt(a)));
  vm->pc++;
}

static void OpNot(VM *vm)
{
  u32 a;
  if (StackSize() < 1) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  StackPush(IntVal(RawVal(a) == 0));
  vm->pc++;
}

static void OpShift(VM *vm)
{
  i32 a, b;
  if (StackSize() < 2) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    RuntimeError("Only integers can be shifted", vm);
    return;
  }
  a = RawInt(a);
  b = RawInt(b);
  if (b < 0) {
    StackPush(IntVal(a >> -b));
  } else {
    StackPush(IntVal(a << b));
  }
  vm->pc++;
}

static void OpXor(VM *vm)
{
  i32 a, b;
  if (StackSize() < 2) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(a) || !IsInt(b)) {
    RuntimeError("Only integers can be xor'd", vm);
    return;
  }
  StackPush(IntVal(RawInt(a) ^ RawInt(b)));
  vm->pc++;
}

static void OpDup(VM *vm)
{
  u32 a;
  MaybeGC(1, vm);
  if (StackSize() < 1) {
    RuntimeError("Stack underflow", vm);
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
    RuntimeError("Stack underflow", vm);
    return;
  }
  StackPop();
  vm->pc++;
}

static void OpSwap(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    RuntimeError("Stack underflow", vm);
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
    RuntimeError("Stack underflow", vm);
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
    RuntimeError("Stack underflow", vm);
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
    RuntimeError("Stack underflow", vm);
    return;
  }
  if (n < 0) {
    RuntimeError("Invalid stack index", vm);
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
    RuntimeError("Stack underflow", vm);
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
    RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  if (!IsPair(a)) {
    RuntimeError("Only pairs have heads", vm);
    return;
  }
  StackPush(Head(a));
  vm->pc++;
}

static void OpTail(VM *vm)
{
  u32 a;
  if (StackSize() < 1) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  if (!IsPair(a)) {
    RuntimeError("Only pairs have tails", vm);
    return;
  }
  StackPush(Tail(a));
  vm->pc++;
}

static void OpTuple(VM *vm)
{
  u32 count = ReadLEB(++vm->pc, vm->program->code);
  vm->pc += LEBSize(count);
  MaybeGC(count+2, vm);
  StackPush(Tuple(count));
}

static void OpLen(VM *vm)
{
  u32 a;
  if (StackSize() < 1) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  if (!IsTuple(a) && !IsBinary(a)) {
    RuntimeError("Only tuples and binaries have lengths", vm);
    return;
  }
  StackPush(IntVal(ObjLength(a)));
  vm->pc++;
}

static void OpGet(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPop();
  a = StackPop();
  if (!IsInt(b)) {
    RuntimeError("Only integers can be indexes", vm);
    return;
  }
  if (RawInt(b) < 0 || RawInt(b) >= (i32)ObjLength(a)) {
    RuntimeError("Out of bounds", vm);
    return;
  }
  if (IsTuple(a)) {
    StackPush(TupleGet(a, RawInt(b)));
  } else if (IsBinary(a)) {
    StackPush(IntVal(BinaryGet(a, RawInt(b))));
  } else {
    RuntimeError("Only tuples and binaries can be accessed", vm);
    return;
  }
  vm->pc++;
}

static void OpSet(VM *vm)
{
  u32 a, b, c;
  if (StackSize() < 3) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  c = StackPop();
  b = StackPop();
  a = StackPop();
  if (!IsInt(b)) {
    RuntimeError("Only integers can be indexes", vm);
    return;
  }
  if (RawInt(b) < 0) {
    RuntimeError("Out of bounds", vm);
    return;
  }
  if (IsTuple(a)) {
    TupleSet(a, RawVal(b), c);
  } else if (IsBinary(a)) {
    BinarySet(a, RawVal(b), c);
  } else {
    RuntimeError("Only tuples and binaries can be accessed", vm);
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
  if (StackSize() < 1) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  a = StackPop();
  if (!IsInt(a)) {
    RuntimeError("Only symbols can become strings", vm);
    return;
  }
  name = SymbolName(RawVal(a));
  if (!name) {
    RuntimeError("Only symbols can become strings", vm);
    return;
  }
  len = StrLen(name);
  MaybeGC(BinSpace(len) + 2, vm);
  StackPush(BinaryFrom(name, len));
  vm->pc++;
}

static void OpJoin(VM *vm)
{
  u32 a, b;
  if (StackSize() < 2) {
    RuntimeError("Stack underflow", vm);
    return;
  }
  b = StackPeek(0);
  a = StackPeek(1);
  if (ValType(a) != ValType(b)) {
    RuntimeError("Only values of the same type can be joined", vm);
    return;
  }
  if (!IsTuple(a) && !IsBinary(a)) {
    RuntimeError("Only tuples and binaries can be joined", vm);
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
        RuntimeError("Stack underflow", vm);
        return;
      }

      b = StackPop();
      a = StackPop();
      StackPush(TupleJoin(a, b));
    } else if (IsBinary(a)) {
      MaybeGC(1 + BinSpace(ObjLength(a) + ObjLength(b)), vm);
      if (StackSize() < 2) {
        RuntimeError("Stack underflow", vm);
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
    RuntimeError("Stack underflow", vm);
    return;
  }
  c = StackPop();
  b = StackPop();
  a = StackPeek(0);
  if (!IsInt(b) || !IsInt(c)) {
    RuntimeError("Only integers can be slice indexes", vm);
    return;
  }
  if (RawInt(b) < 0 || RawInt(c) < RawInt(b) || RawInt(c) > (i32)ObjLength(a)) {
    RuntimeError("Out of bounds", vm);
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
    RuntimeError("Only tuples and binaries can be sliced", vm);
    return;
  }
  vm->pc++;
}

static void OpTrap(VM *vm)
{
  u32 id = ReadLEB(vm->pc+1, vm->program->code);
  u32 value;
  value = PrimitiveFn(id)(vm);
  if (!vm->error) {
    MaybeGC(1, vm);
    StackPush(value);
  }
  vm->pc += LEBSize(id) + 1;
}

typedef void (*OpFn)(VM *vm);
static OpFn ops[128] = {
  /* opNoop */    OpNoop,
  /* opHalt */    OpHalt,
  /* opPanic */   OpPanic,
  /* opConst */   OpConst,
  /* opLookup */  OpLookup,
  /* opDefine */  OpDefine,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* opJump */    OpJump,
  /* opBranch */  OpBranch,
  /* opPos */     OpPos,
  /* opGoto */    OpGoto,
  /* opPush */    OpPush,
  /* opPull */    OpPull,
  /* opLink */    OpLink,
  /* opUnlink */  OpUnlink,
  0, 0, 0, 0, 0, 0, 0, 0,
  /* opAdd */     OpAdd,
  /* opSub */     OpSub,
  /* opMul */     OpMul,
  /* opDiv */     OpDiv,
  /* opRem */     OpRem,
  /* opAnd */     OpAnd,
  /* opOr */      OpOr,
  /* opComp */    OpComp,
  /* opLt */      OpLt,
  /* opGt */      OpGt,
  /* opEq */      OpEq,
  /* opNeg */     OpNeg,
  /* opNot */     OpNot,
  /* opShift */   OpShift,
  /* opXor */     OpXor,
  0,
  /* opDup */     OpDup,
  /* opDrop */    OpDrop,
  /* opSwap */    OpSwap,
  /* opOver */    OpOver,
  /* opRot */     OpRot,
  /* opPick */    OpPick,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* opPair */    OpPair,
  /* opHead */    OpHead,
  /* opTail */    OpTail,
  /* opTuple */   OpTuple,
  /* opLen */     OpLen,
  /* opGet */     OpGet,
  /* opSet */     OpSet,
  /* opStr */     OpStr,
  /* opJoin */    OpJoin,
  /* opSlice */   OpSlice,
  0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* opTrap */    OpTrap,
};

void ExecOp(OpCode op, VM *vm)
{
  ops[op](vm);
}
