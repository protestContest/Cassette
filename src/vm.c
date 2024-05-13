#include "vm.h"
#include "mem.h"
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
  FreeVec(vm->stack);
  InitModuleSymbols(mod);
}

VMStatus VMStep(VM *vm)
{
  return ops[vm->mod->code[vm->pc]](vm);
}

void VMRun(VM *vm)
{
  while (vm->status == ok && vm->pc < (i32)VecCount(vm->mod->code)) {
    vm->status = ops[vm->mod->code[vm->pc++]](vm);
  }
}

#define OneArg() \
  i32 a;\
  if (VecCount(vm->stack) < 1) return stackUnderflow;\
  a = VecPop(vm->stack)

#define TwoArgs() \
  i32 a, b;\
  if (VecCount(vm->stack) < 2) return stackUnderflow;\
  b = VecPop(vm->stack);\
  a = VecPop(vm->stack)

#define ThreeArgs() \
  i32 a, b, c;\
  if (VecCount(vm->stack) < 3) return stackUnderflow;\
  c = VecPop(vm->stack);\
  b = VecPop(vm->stack);\
  a = VecPop(vm->stack)

#define UnaryOp(op)     VecPush(vm->stack, IntVal(op RawVal(a)))
#define BinOp(op)       VecPush(vm->stack, IntVal(RawVal(a) op RawVal(b)))
#define CheckBounds(n)  if ((n) < 0 || (n) > (i32)VecCount(vm->mod->code)) return outOfBounds

static VMStatus OpNoop(VM *vm)
{
  return ok;
}

static VMStatus OpConst(VM *vm)
{
  i32 num = ReadInt(&vm->pc, vm->mod);
  VecPush(vm->stack, vm->mod->constants[num]);
  return ok;
}

static VMStatus OpAdd(VM *vm)
{
  TwoArgs();
  if (ValType(a) != intType || ValType(b) != intType) return invalidType;
  BinOp(+);
  return ok;
}

static VMStatus OpSub(VM *vm)
{
  TwoArgs();
  if (ValType(a) != intType || ValType(b) != intType) return invalidType;
  BinOp(-);
  return ok;
}

static VMStatus OpMul(VM *vm)
{
  TwoArgs();
  if (ValType(a) != intType || ValType(b) != intType) return invalidType;
  BinOp(*);
  return ok;
}

static VMStatus OpDiv(VM *vm)
{
  TwoArgs();
  if (ValType(a) != intType || ValType(b) != intType) return invalidType;
  if (RawVal(b) == 0) return divideByZero;
  BinOp(/);
  return ok;
}

static VMStatus OpRem(VM *vm)
{
  TwoArgs();
  if (ValType(a) != intType || ValType(b) != intType) return invalidType;
  if (RawVal(b) == 0) return divideByZero;
  BinOp(%);
  return ok;
}

static VMStatus OpAnd(VM *vm)
{
  TwoArgs();
  if (ValType(a) != intType || ValType(b) != intType) return invalidType;
  BinOp(&);
  return ok;
}

static VMStatus OpOr(VM *vm)
{
  TwoArgs();
  if (ValType(a) != intType || ValType(b) != intType) return invalidType;
  BinOp(|);
  return ok;
}

static VMStatus OpComp(VM *vm)
{
  OneArg();
  if (ValType(a) != intType) return invalidType;
  UnaryOp(~);
  return ok;
}

static VMStatus OpLt(VM *vm)
{
  TwoArgs();
  if (ValType(a) != intType || ValType(b) != intType) return invalidType;
  BinOp(<);
  return ok;
}

static VMStatus OpGt(VM *vm)
{
  TwoArgs();
  if (ValType(a) != intType || ValType(b) != intType) return invalidType;
  BinOp(>);
  return ok;
}

static VMStatus OpEq(VM *vm)
{
  TwoArgs();
  VecPush(vm->stack, IntVal(a == b));
  return ok;
}

static VMStatus OpNeg(VM *vm)
{
  OneArg();
  if (ValType(a) != intType) return invalidType;
  UnaryOp(-);
  return ok;
}

static VMStatus OpNot(VM *vm)
{
  OneArg();
  VecPush(vm->stack, IntVal(RawVal(a) != 0));
  return ok;
}

static VMStatus OpShift(VM *vm)
{
  TwoArgs();
  if (ValType(a) != intType || ValType(b) != intType) return invalidType;
  BinOp(<<);
  return ok;
}

static VMStatus OpNil(VM *vm)
{
  VecPush(vm->stack, 0);
  return ok;
}

static VMStatus OpPair(VM *vm)
{
  TwoArgs();
  VecPush(vm->stack, Pair(a, b));
  return ok;
}

static VMStatus OpHead(VM *vm)
{
  OneArg();
  if (ValType(a) != pairType) return invalidType;
  VecPush(vm->stack, Head(a));
  return ok;
}

static VMStatus OpTail(VM *vm)
{
  OneArg();
  if (ValType(a) != pairType) return invalidType;
  VecPush(vm->stack, Tail(a));
  return ok;
}

static VMStatus OpTuple(VM *vm)
{
  i32 count = ReadInt(&vm->pc, vm->mod);
  VecPush(vm->stack, Tuple(count));
  return ok;
}

static VMStatus OpLen(VM *vm)
{
  OneArg();
  if (ValType(a) != objType) return invalidType;
  VecPush(vm->stack, ObjLength(a));
  return ok;
}

static VMStatus OpGet(VM *vm)
{
  TwoArgs();
  if (ValType(a) != objType) return invalidType;
  if (ValType(b) != intType) return invalidType;
  VecPush(vm->stack, ObjGet(a, RawVal(b)));
  return ok;
}

static VMStatus OpSet(VM *vm)
{
  ThreeArgs();
  if (ValType(a) != objType) return invalidType;
  if (ValType(b) != intType) return invalidType;
  ObjSet(a, RawVal(b), c);
  return ok;
}

static VMStatus OpStr(VM *vm)
{
  return undefined;
}

static VMStatus OpBin(VM *vm)
{
  i32 count = ReadInt(&vm->pc, vm->mod);
  VecPush(vm->stack, Binary(count));
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
  i32 a;
  if (VecCount(vm->stack) < 1) return stackUnderflow;
  a = VecPop(vm->stack);
  if (RawVal(a)) {
    CheckBounds(vm->pc + n);
    vm->pc += n;
  }
  return ok;
}

static VMStatus OpTrap(VM *vm)
{
  i32 n = ReadInt(&vm->pc, vm->mod);
  return n;
}

static VMStatus OpPos(VM *vm)
{
  VecPush(vm->stack, IntVal(vm->pc));
  return ok;
}

static VMStatus OpGoto(VM *vm)
{
  OneArg();
  if (ValType(a) != intType) return invalidType;
  CheckBounds((i32)RawVal(a));
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
  VecPush(vm->stack, a);
  VecPush(vm->stack, a);
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
  VecPush(vm->stack, b);
  VecPush(vm->stack, a);
  return ok;
}

static VMStatus OpOver(VM *vm)
{
  TwoArgs();
  VecPush(vm->stack, a);
  VecPush(vm->stack, b);
  VecPush(vm->stack, a);
  return ok;
}

static VMStatus OpRot(VM *vm)
{
  ThreeArgs();
  VecPush(vm->stack, b);
  VecPush(vm->stack, c);
  VecPush(vm->stack, a);
  return ok;
}

static VMStatus OpGetEnv(VM *vm)
{
  VecPush(vm->stack, vm->env);
  return ok;
}

static VMStatus OpSetEnv(VM *vm)
{
  if (VecCount(vm->stack) < 1) return stackUnderflow;
  vm->env = VecPop(vm->stack);
  return ok;
}

static VMStatus OpLookup(VM *vm)
{
  i32 n = ReadInt(&vm->pc, vm->mod);
  i32 env = vm->env;
  if (n < 0) return undefined;
  while (env) {
    i32 frame = Head(env);
    if (n < ObjLength(frame)) {
      VecPush(vm->stack, ObjGet(frame, n));
      return ok;
    }
    n -= ObjLength(frame);
    env = Tail(env);
  }
  return undefined;
}

static VMStatus OpDefine(VM *vm)
{
  i32 n = ReadInt(&vm->pc, vm->mod);
  i32 frame = Head(vm->env);
  i32 a;
  if (VecCount(vm->stack) < 1) return stackUnderflow;
  a = VecPop(vm->stack);
  if (n < 0 || n >= ObjLength(frame)) return outOfBounds;
  ObjSet(frame, n, a);
  return ok;
}
