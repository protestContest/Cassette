#include "vm.h"
#include "ops.h"
#include "compile/compile.h"
#include "env.h"
#include "primitives.h"
#include "univ/str.h"
#include "univ/math.h"
#include "univ/system.h"

#ifdef DEBUG
#include "debug.h"
#endif

static bool CheckMem(VM *vm, u32 amount);

void InitVM(VM *vm)
{
  vm->pc = 0;
  vm->chunk = 0;
  InitVec((Vec*)&vm->stack, sizeof(Val), 256);
  InitMem(&vm->mem, 1024);

  vm->stack.count = 1;
  Env(vm) = ExtendEnv(Nil, PrimitiveEnv(&vm->mem), &vm->mem);
}

void DestroyVM(VM *vm)
{
  vm->pc = 0;
  vm->chunk = 0;
  DestroyVec((Vec*)&vm->stack);
  DestroyMem(&vm->mem);
}

Result RunChunk(Chunk *chunk, VM *vm)
{
  vm->chunk = chunk;
  Seed(Ticks());

#ifdef DEBUG
  PrintTraceHeader();
#endif

  while (vm->pc < chunk->code.count) {
    OpCode op = ChunkRef(chunk, vm->pc);

#ifdef DEBUG
    TraceInstruction(op, vm);
#endif

    switch (op) {
    case OpNoop:
      vm->pc += OpLength(op);
      break;
    case OpHalt:
      vm->pc = chunk->code.count;
      break;
    case OpError:
      if (IsSym(StackRef(vm, 0))) {
        return RuntimeError(SymbolName(StackRef(vm, 0), &chunk->symbols), vm);
      } else {
        return RuntimeError("Unknown error", vm);
      }
      break;
    case OpPop:
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpDup:
      StackPush(vm, StackRef(vm, 0));
      vm->pc += OpLength(op);
      break;
    case OpConst:
      StackPush(vm, ChunkConst(chunk, vm->pc+1));
      vm->pc += OpLength(op);
      break;
    case OpNeg:
      if (IsInt(StackRef(vm, 0))) {
        StackRef(vm, 0) = IntVal(-RawInt(StackRef(vm, 0)));
      } else if (IsFloat(StackRef(vm, 0))) {
        StackRef(vm, 0) = FloatVal(-RawFloat(StackRef(vm, 0)));
      } else {
        return RuntimeError("Negative is only defined for numbers", vm);
      }
      vm->pc += OpLength(op);
      break;
    case OpNot:
      if (StackRef(vm, 0) == False || StackRef(vm, 0) == Nil) {
        StackRef(vm, 0) = True;
      } else {
        StackRef(vm, 0) = False;
      }
      vm->pc += OpLength(op);
      break;
    case OpLen:
      if (IsPair(StackRef(vm, 0))) {
        StackRef(vm, 0) = IntVal(ListLength(StackRef(vm, 0), &vm->mem));
      } else if (IsTuple(StackRef(vm, 0), &vm->mem)) {
        StackRef(vm, 0) = IntVal(TupleLength(StackRef(vm, 0), &vm->mem));
      } else if (IsBinary(StackRef(vm, 0), &vm->mem)) {
        StackRef(vm, 0) = IntVal(BinaryLength(StackRef(vm, 0), &vm->mem));
      } else {
        return RuntimeError("Length is only defined for collections", vm);
      }
      vm->pc += OpLength(op);
      break;
    case OpMul:
      if (IsInt(StackRef(vm, 0)) && IsInt(StackRef(vm, 1))) {
        i32 a = RawInt(StackRef(vm, 1));
        i32 b = RawInt(StackRef(vm, 0));
        if (b != 0 && a > RawInt(MaxIntVal) / b) return RuntimeError("Arithmetic overflow", vm);
        if (b != 0 && a < RawInt(MinIntVal) / b) return RuntimeError("Arithmetic underflow", vm);
        StackRef(vm, 1) = IntVal(a*b);
      } else if (IsNum(StackRef(vm, 0)) && IsNum(StackRef(vm, 1))) {
        StackRef(vm, 1) = FloatVal(RawNum(StackRef(vm, 1)) * RawNum(StackRef(vm, 0)));
      } else {
        return RuntimeError("Multiplication is only defined for numbers", vm);
      }
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpDiv:
      if (IsNum(StackRef(vm, 0)) && IsNum(StackRef(vm, 1))) {
        if ((float)RawNum(StackRef(vm, 0)) == 0.0) return RuntimeError("Divide by zero", vm);
        StackRef(vm, 1) = FloatVal((float)RawNum(StackRef(vm, 1)) / (float)RawNum(StackRef(vm, 0)));
      } else {
        return RuntimeError("Division is only defined for numbers", vm);
      }
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpRem:
      if (IsInt(StackRef(vm, 0)) && IsInt(StackRef(vm, 1))) {
        StackRef(vm, 1) = IntVal(RawInt(StackRef(vm, 1)) % RawInt(StackRef(vm, 0)));
      } else {
        return RuntimeError("Remainder is only defined for numbers", vm);
      }
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpAdd:
      if (IsInt(StackRef(vm, 0)) && IsInt(StackRef(vm, 1))) {
        i32 a = RawInt(StackRef(vm, 1));
        i32 b = RawInt(StackRef(vm, 0));
        if (b > 0 && a > RawInt(MaxIntVal) - b) return RuntimeError("Arithmetic overflow", vm);
        if (b < 0 && a < RawInt(MinIntVal) - b) return RuntimeError("Arithmetic underflow", vm);
        StackRef(vm, 1) = IntVal(a + b);
      } else if (IsNum(StackRef(vm, 0)) && IsNum(StackRef(vm, 1))) {
        StackRef(vm, 1) = FloatVal(RawNum(StackRef(vm, 1)) + RawNum(StackRef(vm, 0)));
      } else {
        return RuntimeError("Addition is only defined for numbers", vm);
      }
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpSub:
      if (IsInt(StackRef(vm, 0)) && IsInt(StackRef(vm, 1))) {
        i32 a = RawInt(StackRef(vm, 1));
        i32 b = RawInt(StackRef(vm, 0));
        if (b > 0 && a < RawInt(MinIntVal) + b) return RuntimeError("Arithmetic underflow", vm);
        if (b < 0 && a > RawInt(MinIntVal) + b) return RuntimeError("Arithmetic underflow", vm);
        StackRef(vm, 1) = IntVal(RawInt(StackRef(vm, 1)) - RawInt(StackRef(vm, 0)));
      } else if (IsNum(StackRef(vm, 0)) && IsNum(StackRef(vm, 1))) {
        StackRef(vm, 1) = FloatVal(RawNum(StackRef(vm, 1)) - RawNum(StackRef(vm, 0)));
      } else {
        return RuntimeError("Subtraction is only defined for numbers", vm);
      }
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpIn:
      if (IsPair(StackRef(vm, 0))) {
        StackRef(vm, 1) = BoolVal(ListContains(StackRef(vm, 0), StackRef(vm, 1), &vm->mem));
      } else if (IsTuple(StackRef(vm, 0), &vm->mem)) {
        StackRef(vm, 1) = BoolVal(TupleContains(StackRef(vm, 0), StackRef(vm, 1), &vm->mem));
      } else if (IsBinary(StackRef(vm, 0), &vm->mem)) {
        StackRef(vm, 1) = BoolVal(BinaryContains(StackRef(vm, 0), StackRef(vm, 1), &vm->mem));
      } else {
        StackRef(vm, 1) = False;
      }
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpLt:
      if (IsNum(StackRef(vm, 0)) && IsNum(StackRef(vm, 1))) {
        StackRef(vm, 1) = BoolVal(RawNum(StackRef(vm, 1)) < RawNum(StackRef(vm, 0)));
      } else {
        return RuntimeError("Less than is only defined for numbers", vm);
      }
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpGt:
      if (IsNum(StackRef(vm, 0)) && IsNum(StackRef(vm, 1))) {
        StackRef(vm, 1) = BoolVal(RawNum(StackRef(vm, 1)) > RawNum(StackRef(vm, 0)));
      } else {
        return RuntimeError("Greater than is only defined for numbers", vm);
      }
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpEq:
      StackRef(vm, 1) = BoolVal(StackRef(vm, 1) == StackRef(vm, 0));
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpStr: {
      char *str;
      u32 size;
      if (!IsSym(StackRef(vm, 0))) return RuntimeError("Strings must be made from symbols", vm);

      str = SymbolName(StackRef(vm, 0), &chunk->symbols);
      size = StrLen(str);

      if (!CheckMem(vm, NumBinCells(size))) return RuntimeError("Out of memory", vm);

      StackRef(vm, 0) = BinaryFrom(str, StrLen(str), &vm->mem);
      vm->pc += OpLength(op);
      break;
    }
    case OpPair:
      if (!CheckMem(vm, 2)) return RuntimeError("Out of memory", vm);
      StackRef(vm, 1) = Pair(StackRef(vm, 0), StackRef(vm, 1), &vm->mem);
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpTuple:
      if (!CheckMem(vm, RawInt(StackRef(vm, 0)) + 1)) return RuntimeError("Out of memory", vm);
      StackRef(vm, 0) = MakeTuple(RawInt(StackRef(vm, 0)), &vm->mem);
      vm->pc += OpLength(op);
      break;
    case OpSet:
      if (!IsTuple(StackRef(vm, 0), &vm->mem)) return RuntimeError("Set is only defined for tuples", vm);
      if (TupleLength(StackRef(vm, 0), &vm->mem) <= (u32)RawInt(ChunkConst(chunk, vm->pc+1))) return RuntimeError("Out of bounds", vm);

      TupleSet(StackRef(vm, 0), RawInt(ChunkConst(chunk, vm->pc+1)), StackRef(vm, 1), &vm->mem);
      StackRef(vm, 1) = StackRef(vm, 0);
      StackPop(vm);

      vm->pc += OpLength(op);
      break;
    case OpGet:
      if (!IsInt(StackRef(vm, 0))) return RuntimeError("Index must be an integer", vm);
      if (RawInt(StackRef(vm, 0)) < 0) return RuntimeError("Out of bounds", vm);

      if (IsPair(StackRef(vm, 1))) {
        StackRef(vm, 1) = ListGet(StackRef(vm, 1), RawInt(StackRef(vm, 0)), &vm->mem);
      } else if (IsTuple(StackRef(vm, 1), &vm->mem)) {
        u32 index = RawInt(StackRef(vm, 0));
        Val tuple = StackRef(vm, 1);
        if (index >= TupleLength(tuple, &vm->mem)) return RuntimeError("Out of bounds", vm);
        StackRef(vm, 1) = TupleGet(tuple, index, &vm->mem);
      } else if (IsBinary(StackRef(vm, 1), &vm->mem)) {
        u32 index = RawInt(StackRef(vm, 0));
        Val binary = StackRef(vm, 1);
        if (index >= BinaryLength(binary, &vm->mem)) return RuntimeError("Out of bounds", vm);
        StackRef(vm, 1) = IntVal(((u8*)BinaryData(binary, &vm->mem))[index]);
      } else {
        return RuntimeError("Access is only defined for collections", vm);
      }
      vm->stack.count--;
      vm->pc += OpLength(op);
      break;
    case OpCat:
      if (IsPair(StackRef(vm, 0)) && IsPair(StackRef(vm, 1))) {
        StackRef(vm, 1) = ListCat(StackRef(vm, 1), StackRef(vm, 0), &vm->mem);
      } else if (IsTuple(StackRef(vm, 0), &vm->mem) && IsTuple(StackRef(vm, 1), &vm->mem)) {
        StackRef(vm, 1) = TupleCat(StackRef(vm, 1), StackRef(vm, 0), &vm->mem);
      } else if (IsBinary(StackRef(vm, 0), &vm->mem) && IsBinary(StackRef(vm, 1), &vm->mem)) {
        StackRef(vm, 1) = BinaryCat(StackRef(vm, 1), StackRef(vm, 0), &vm->mem);
      } else {
        return RuntimeError("Concatenation is only defined for collections of the same type", vm);
      }
      vm->stack.count--;
      vm->pc += OpLength(op);
      break;
    case OpExtend:
      if (!IsTuple(StackRef(vm, 0), &vm->mem)) return RuntimeError("Frame must be a tuple", vm);
      if (!CheckMem(vm, 2)) return RuntimeError("Out of memory", vm);

      Env(vm) = ExtendEnv(Env(vm), StackRef(vm, 0), &vm->mem);
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpDefine:
      Define(StackRef(vm, 0), RawInt(ChunkConst(chunk, vm->pc+1)), Env(vm), &vm->mem);
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpLookup:
      StackPush(vm, Lookup(RawInt(ChunkConst(chunk, vm->pc+1)), RawInt(ChunkConst(chunk, vm->pc+2)), Env(vm), &vm->mem));
      if (StackRef(vm, 0) == Undefined) return RuntimeError("Undefined variable", vm);
      vm->pc += OpLength(op);
      break;
    case OpExport:
      StackPush(vm, Head(Env(vm), &vm->mem));
      Env(vm) = Tail(Env(vm), &vm->mem);
      vm->pc += OpLength(op);
      break;
    case OpJump:
      vm->pc += RawInt(ChunkConst(chunk, vm->pc+1));
      break;
    case OpBranch:
      if (StackRef(vm, 0) == Nil || StackRef(vm, 0) == False) {
        vm->pc += OpLength(op);
      } else {
        vm->pc += RawInt(ChunkConst(chunk, vm->pc+1));
      }
      break;
    case OpLink:
      StackPush(vm, Env(vm));
      StackPush(vm, IntVal(vm->pc + ChunkConst(chunk, vm->pc+1)));
      vm->pc += OpLength(op);
      break;
    case OpReturn:
      vm->pc = RawInt(StackRef(vm, 1));
      Env(vm) = StackRef(vm, 2);
      StackRef(vm, 2) = StackRef(vm, 0);
      vm->stack.count -= 2;
      break;
    case OpApply:
      if (IsPair(StackRef(vm, 0)) && Head(StackRef(vm, 0), &vm->mem) == Function) {
        /* normal function */
        Val num_args = ChunkConst(chunk, vm->pc+1);
        vm->pc = RawInt(Head(Tail(StackRef(vm, 0), &vm->mem), &vm->mem));
        Env(vm) = Tail(Tail(StackRef(vm, 0), &vm->mem), &vm->mem);
        StackRef(vm, 0) = num_args;
      } else if (IsPair(StackRef(vm, 0)) && Head(StackRef(vm, 0), &vm->mem) == Primitive) {
          /* apply primitive */
        Val num_args = ChunkConst(chunk, vm->pc+1);
        Val prim = StackPop(vm);
        Result result = DoPrimitive(Tail(prim, &vm->mem), RawInt(num_args), vm);
        if (!result.ok) return result;
        vm->pc = RawInt(StackPop(vm));
        Env(vm) = StackPop(vm);
        StackPush(vm, result.value);
      } else {
        /* not a function, just return */
        vm->pc = RawInt(StackRef(vm, 1));
        Env(vm) = StackRef(vm, 2);
        StackRef(vm, 2) = StackRef(vm, 0);
        vm->stack.count -= 2;
      }
      break;
    default:
      return RuntimeError("Undefined op", vm);
    }
  }

#ifdef DEBUG
    TraceInstruction(OpHalt, vm);
#endif

  return OkResult(Nil);
}

static bool CheckMem(VM *vm, u32 amount)
{
  if (!CheckCapacity(&vm->mem, amount)) {
    CollectGarbage(vm->stack.items, vm->stack.count, &vm->mem);
  }

  if (!CheckCapacity(&vm->mem, amount)) {
    ResizeMem(&vm->mem, 2*vm->mem.capacity);
  }

  return CheckCapacity(&vm->mem, amount);
}

Result RuntimeError(char *message, VM *vm)
{
  char *filename = ChunkFile(vm->pc, vm->chunk);
  u32 pos = GetSourcePosition(vm->pc, vm->chunk);
  return ErrorResult(message, filename, pos);
}
