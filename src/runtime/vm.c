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

static Result RunInstruction(VM *vm);
static bool CheckMem(VM *vm, u32 amount);

void InitVM(VM *vm, Chunk *chunk)
{
  vm->pc = 0;
  vm->chunk = chunk;
  vm->trace = false;
  InitVec((Vec*)&vm->stack, sizeof(Val), 256);
  InitMem(&vm->mem, 1024);
  InitVec((Vec*)&vm->canvases, sizeof(void*), 8);

  vm->stack.count = 1;
  Env(vm) = Nil;
}

void DestroyVM(VM *vm)
{
  vm->pc = 0;
  vm->chunk = 0;
  DestroyVec((Vec*)&vm->stack);
  DestroyMem(&vm->mem);
}

Result Run(VM *vm, u32 num_instructions)
{
  u32 i;
  Result result = OkResult(BoolVal(vm->pc < vm->chunk->code.count));
  if (!vm->chunk) return RuntimeError("VM not initialized", vm);

  for (i = 0; i < num_instructions && result.value == True; i++) {
    result = RunInstruction(vm);
    if (!result.ok) return result;
  }

  return result;
}

Result RunChunk(Chunk *chunk, VM *vm)
{
  Result result = OkResult(Nil);
  vm->pc = 0;
  vm->chunk = chunk;

#ifdef DEBUG
  if (vm->trace) PrintTraceHeader();
#endif

  while (vm->pc < chunk->code.count && result.ok) {
    result = RunInstruction(vm);
  }

#ifdef DEBUG
  if (vm->trace) TraceInstruction(OpHalt, vm);
#endif

  return result;
}

static Result RunInstruction(VM *vm)
{
  OpCode op = ChunkRef(vm->chunk, vm->pc);

#ifdef DEBUG
  if (vm->trace) TraceInstruction(op, vm);
#endif

  switch (op) {
  case OpNoop:
    vm->pc += OpLength(op);
    break;
  case OpHalt:
    vm->pc = vm->chunk->code.count;
    break;
  case OpError:
    if (vm->stack.count > 0 && IsSym(StackRef(vm, 0))) {
      return RuntimeError(SymbolName(StackRef(vm, 0), &vm->chunk->symbols), vm);
    } else {
      return RuntimeError("Unknown error", vm);
    }
    break;
  case OpPop:
    if (vm->stack.count < 1) return RuntimeError("Stack underflow", vm);
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpSwap:
    if (vm->stack.count >= 2) {
      Val tmp = StackRef(vm, 0);
      StackRef(vm, 0) = StackRef(vm, 1);
      StackRef(vm, 1) = tmp;
    }
    vm->pc += OpLength(op);
    break;
  case OpDup:
    if (vm->stack.count + 1 > vm->stack.capacity) return RuntimeError("Stack overflow", vm);
    StackPush(vm, StackRef(vm, 0));
    vm->pc += OpLength(op);
    break;
  case OpConst:
    if (vm->stack.count + 1 > vm->stack.capacity) return RuntimeError("Stack overflow", vm);
    StackPush(vm, ChunkConst(vm->chunk, vm->pc+1));
    vm->pc += OpLength(op);
    break;
  case OpNeg:
    if (vm->stack.count < 1) return RuntimeError("Stack underflow", vm);
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
    if (vm->stack.count < 1) return RuntimeError("Stack underflow", vm);
    if (StackRef(vm, 0) == False || StackRef(vm, 0) == Nil) {
      StackRef(vm, 0) = True;
    } else {
      StackRef(vm, 0) = False;
    }
    vm->pc += OpLength(op);
    break;
  case OpBitNot:
    if (vm->stack.count < 1) return RuntimeError("Stack underflow", vm);
    if (!IsInt(StackRef(vm, 0))) return RuntimeError("Bitwise ops are only defined for integers", vm);
    StackRef(vm, 0) = IntVal(~RawInt(StackRef(vm, 0)));
    vm->pc += OpLength(op);
    break;
  case OpLen:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    if (IsPair(StackRef(vm, 0))) {
      StackRef(vm, 0) = IntVal(ListLength(StackRef(vm, 0), &vm->mem));
    } else if (IsTuple(StackRef(vm, 0), &vm->mem)) {
      StackRef(vm, 0) = IntVal(TupleLength(StackRef(vm, 0), &vm->mem));
    } else if (IsBinary(StackRef(vm, 0), &vm->mem)) {
      StackRef(vm, 0) = IntVal(BinaryLength(StackRef(vm, 0), &vm->mem));
    } else if (IsMap(StackRef(vm, 0), &vm->mem)) {
      StackRef(vm, 0) = IntVal(MapCount(StackRef(vm, 0), &vm->mem));
    } else {
      return RuntimeError("Length is only defined for collections", vm);
    }
    vm->pc += OpLength(op);
    break;
  case OpMul:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    if (!IsNum(StackRef(vm, 1), &vm->mem) || !IsNum(StackRef(vm, 0), &vm->mem)) {
      return RuntimeError("Multiplication is only defined for numbers", vm);
    } else if (IsInt(StackRef(vm, 1)) && IsInt(StackRef(vm, 0))) {
      StackRef(vm, 1) = IntVal(RawInt(StackRef(vm, 1)) * RawInt(StackRef(vm, 0)));
    } else {
      StackRef(vm, 1) = FloatVal(RawNum(StackRef(vm, 1)) * RawNum(StackRef(vm, 0)));
    }
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpDiv:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    if (!IsNum(StackRef(vm, 1), &vm->mem) || !IsNum(StackRef(vm, 0), &vm->mem)) {
      return RuntimeError("Division is only defined for numbers", vm);
    } else {
      StackRef(vm, 1) = FloatVal(RawNum(StackRef(vm, 1)) / RawNum(StackRef(vm, 0)));
    }
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpRem:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    if (!IsInt(StackRef(vm, 1)) || !IsInt(StackRef(vm, 0))) {
      return RuntimeError("Division is only defined for integers", vm);
    } else  {
      StackRef(vm, 1) = IntVal(RawInt(StackRef(vm, 1)) % RawInt(StackRef(vm, 0)));
    }
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpAdd:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    if (!IsNum(StackRef(vm, 1), &vm->mem) || !IsNum(StackRef(vm, 0), &vm->mem)) {
      return RuntimeError("Addition is only defined for numbers", vm);
    } else if (IsInt(StackRef(vm, 1)) && IsInt(StackRef(vm, 0))) {
      StackRef(vm, 1) = IntVal(RawInt(StackRef(vm, 1)) + RawInt(StackRef(vm, 0)));
    } else {
      StackRef(vm, 1) = FloatVal(RawNum(StackRef(vm, 1)) + RawNum(StackRef(vm, 0)));
    }
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpSub:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    if (!IsNum(StackRef(vm, 1), &vm->mem) || !IsNum(StackRef(vm, 0), &vm->mem)) {
      return RuntimeError("Subtraction is only defined for numbers", vm);
    } else if (IsInt(StackRef(vm, 1)) && IsInt(StackRef(vm, 0))) {
      StackRef(vm, 1) = IntVal(RawInt(StackRef(vm, 1)) - RawInt(StackRef(vm, 0)));
    } else {
      StackRef(vm, 1) = FloatVal(RawNum(StackRef(vm, 1)) - RawNum(StackRef(vm, 0)));
    }
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpShift:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    if (!IsInt(StackRef(vm, 0)) || !IsInt(StackRef(vm, 1))) return RuntimeError("Bit shift is only defined for integers", vm);
    if (RawInt(StackRef(vm, 0)) < 0) {
      StackRef(vm, 1) = IntVal(RawInt(StackRef(vm, 1)) >> -RawInt(StackRef(vm, 0)));
    } else {
      StackRef(vm, 1) = IntVal(RawInt(StackRef(vm, 1)) << RawInt(StackRef(vm, 0)));
    }
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpBitAnd:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    if (!IsInt(StackRef(vm, 0)) || !IsInt(StackRef(vm, 1))) return RuntimeError("Bitwise and is only defined for integers", vm);
    StackRef(vm, 1) = IntVal(RawInt(StackRef(vm, 0)) & RawInt(StackRef(vm, 1)));
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpBitOr:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    if (!IsInt(StackRef(vm, 0)) || !IsInt(StackRef(vm, 1))) return RuntimeError("Bitwise or is only defined for integers", vm);
    StackRef(vm, 1) = IntVal(RawInt(StackRef(vm, 0)) | RawInt(StackRef(vm, 1)));
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpIn:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    if (IsPair(StackRef(vm, 0))) {
      StackRef(vm, 1) = BoolVal(ListContains(StackRef(vm, 0), StackRef(vm, 1), &vm->mem));
    } else if (IsTuple(StackRef(vm, 0), &vm->mem)) {
      StackRef(vm, 1) = BoolVal(TupleContains(StackRef(vm, 0), StackRef(vm, 1), &vm->mem));
    } else if (IsBinary(StackRef(vm, 0), &vm->mem)) {
      StackRef(vm, 1) = BoolVal(BinaryContains(StackRef(vm, 0), StackRef(vm, 1), &vm->mem));
    } else if (IsMap(StackRef(vm, 0), &vm->mem)) {
      StackRef(vm, 1) = BoolVal(MapContains(StackRef(vm, 0), StackRef(vm, 1), &vm->mem));
    } else {
      StackRef(vm, 1) = False;
    }
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpLt:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    if (!IsNum(StackRef(vm, 1), &vm->mem) || !IsNum(StackRef(vm, 0), &vm->mem)) {
      return RuntimeError("Less than is only defined for numbers", vm);
    } else {
      StackRef(vm, 1) = BoolVal(RawNum(StackRef(vm, 1)) < RawNum(StackRef(vm, 0)));
    }
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpGt:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    if (!IsNum(StackRef(vm, 1), &vm->mem) || !IsNum(StackRef(vm, 0), &vm->mem)) {
      return RuntimeError("Greater than is only defined for numbers", vm);
    } else {
      StackRef(vm, 1) = BoolVal(RawNum(StackRef(vm, 1)) > RawNum(StackRef(vm, 0)));
    }
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpEq:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    StackRef(vm, 1) = BoolVal(ValEqual(StackRef(vm, 1), StackRef(vm, 0), &vm->mem));
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpStr: {
    char *str;
    u32 size;
    if (vm->stack.count < 1) return RuntimeError("Stack underflow", vm);
    if (!IsSym(StackRef(vm, 0))) return RuntimeError("Strings must be made from symbols", vm);

    str = SymbolName(StackRef(vm, 0), &vm->chunk->symbols);
    size = StrLen(str);

    if (!CheckMem(vm, NumBinCells(size))) return RuntimeError("Out of memory", vm);

    StackRef(vm, 0) = BinaryFrom(str, StrLen(str), &vm->mem);
    vm->pc += OpLength(op);
    break;
  }
  case OpUnpair: {
    Val pair;
    if (vm->stack.count < 1) return RuntimeError("Stack underflow", vm);
    if (!IsPair(StackRef(vm, 0))) return RuntimeError("Unpair only works on pairs", vm);
    pair = StackPop(vm);
    StackPush(vm, Tail(pair, &vm->mem));
    StackPush(vm, Head(pair, &vm->mem));
    vm->pc += OpLength(op);
    break;
  }
  case OpPair:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    if (!CheckMem(vm, 2)) return RuntimeError("Out of memory", vm);
    StackRef(vm, 1) = Pair(StackRef(vm, 0), StackRef(vm, 1), &vm->mem);
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpTuple:
    if (vm->stack.count < 1) return RuntimeError("Stack underflow", vm);
    if (!CheckMem(vm, RawInt(StackRef(vm, 0)) + 1)) return RuntimeError("Out of memory", vm);
    StackRef(vm, 0) = MakeTuple(RawInt(StackRef(vm, 0)), &vm->mem);
    vm->pc += OpLength(op);
    break;
  case OpMap: {
    Val keys, values, map;
    u32 i;
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    keys = StackPop(vm);
    values = StackPop(vm);
    if (!IsTuple(keys, &vm->mem) || !IsTuple(values, &vm->mem)) {
      return RuntimeError("Expected map keys and values to be tuples", vm);
    }
    if (TupleLength(keys, &vm->mem) != TupleLength(values, &vm->mem)) {
      return RuntimeError("Maps must have the same number of keys and values", vm);
    }
    map = MakeMap(&vm->mem);
    for (i = 0; i < TupleLength(keys, &vm->mem); i++) {
      map = MapSet(map, TupleGet(keys, i, &vm->mem), TupleGet(values, i, &vm->mem), &vm->mem);
    }
    StackPush(vm, map);
    vm->pc += OpLength(op);
    break;
  }
  case OpSet:
    if (vm->stack.count < 3) return RuntimeError("Stack underflow", vm);
    if (IsTuple(StackRef(vm, 2), &vm->mem)) {
      if (!IsInt(StackRef(vm, 0))) return RuntimeError("Tuple indexes must be integers", vm);
      if (TupleLength(StackRef(vm, 2), &vm->mem) <= (u32)RawInt(StackRef(vm, 0))) return RuntimeError("Out of bounds", vm);
      TupleSet(StackRef(vm, 2), RawInt(StackRef(vm, 0)), StackRef(vm, 1), &vm->mem);
    } else if (IsMap(StackRef(vm, 2), &vm->mem)) {
      StackRef(vm, 2) = MapSet(StackRef(vm, 2), StackRef(vm, 0), StackRef(vm, 1), &vm->mem);
    } else {
      return RuntimeError("Set is only defined for tuples and maps", vm);
    }

    StackPop(vm);
    StackPop(vm);

    vm->pc += OpLength(op);
    break;
  case OpGet:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);

    if (IsPair(StackRef(vm, 1))) {
      if (!IsInt(StackRef(vm, 0))) return RuntimeError("Index for a list must be an integer", vm);
      if (RawInt(StackRef(vm, 0)) < 0) return RuntimeError("Out of bounds", vm);
      StackRef(vm, 1) = ListGet(StackRef(vm, 1), RawInt(StackRef(vm, 0)), &vm->mem);
    } else if (IsTuple(StackRef(vm, 1), &vm->mem)) {
      u32 index = RawInt(StackRef(vm, 0));
      Val tuple = StackRef(vm, 1);
      if (!IsInt(StackRef(vm, 0))) return RuntimeError("Index for a tuple must be an integer", vm);
      if (index < 0 || index >= TupleLength(tuple, &vm->mem)) return RuntimeError("Out of bounds", vm);
      StackRef(vm, 1) = TupleGet(tuple, index, &vm->mem);
    } else if (IsBinary(StackRef(vm, 1), &vm->mem)) {
      u32 index = RawInt(StackRef(vm, 0));
      Val binary = StackRef(vm, 1);
      if (!IsInt(StackRef(vm, 0))) return RuntimeError("Index for a binary must be an integer", vm);
      if (index < 0 || index >= BinaryLength(binary, &vm->mem)) return RuntimeError("Out of bounds", vm);
      StackRef(vm, 1) = IntVal(((u8*)BinaryData(binary, &vm->mem))[index]);
    } else if (IsMap(StackRef(vm, 1), &vm->mem)) {
      Val key = StackRef(vm, 0);
      if (!IsSym(key)) return RuntimeError("Map access syntax must use a symbol key", vm);
      if (!MapContains(StackRef(vm, 1), key, &vm->mem)) return RuntimeError("Undefined map key", vm);
      StackRef(vm, 1) = MapGet(StackRef(vm, 1), key, &vm->mem);
    } else {
      return RuntimeError("Access is only defined for collections", vm);
    }
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpCat:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    if (IsPair(StackRef(vm, 0)) && IsPair(StackRef(vm, 1))) {
      u32 len = 2*ListLength(StackRef(vm, 1), &vm->mem);
      if (!CheckMem(vm, len)) return RuntimeError("Out of memory", vm);
      StackRef(vm, 1) = ListCat(StackRef(vm, 1), StackRef(vm, 0), &vm->mem);
    } else if (IsTuple(StackRef(vm, 0), &vm->mem) && IsTuple(StackRef(vm, 1), &vm->mem)) {
      u32 len = TupleLength(StackRef(vm, 0), &vm->mem) + TupleLength(StackRef(vm, 1), &vm->mem);
      if (!CheckMem(vm, len)) return RuntimeError("Out of memory", vm);
      StackRef(vm, 1) = TupleCat(StackRef(vm, 1), StackRef(vm, 0), &vm->mem);
    } else if (IsBinary(StackRef(vm, 0), &vm->mem) && IsBinary(StackRef(vm, 1), &vm->mem)) {
      u32 len = NumBinCells(BinaryLength(StackRef(vm, 0), &vm->mem)) + NumBinCells(BinaryLength(StackRef(vm, 1), &vm->mem));
      if (!CheckMem(vm, len)) return RuntimeError("Out of memory", vm);
      StackRef(vm, 1) = BinaryCat(StackRef(vm, 1), StackRef(vm, 0), &vm->mem);
    } else if (IsMap(StackRef(vm, 0), &vm->mem) && IsMap(StackRef(vm, 1), &vm->mem)) {
      StackRef(vm, 1) = MapMerge(StackRef(vm, 1), StackRef(vm, 0), &vm->mem);
    } else {
      return RuntimeError("Concatenation is only defined for collections of the same type", vm);
    }
    vm->stack.count--;
    vm->pc += OpLength(op);
    break;
  case OpExtend:
    if (vm->stack.count < 1) return RuntimeError("Stack underflow", vm);
    if (!IsTuple(StackRef(vm, 0), &vm->mem)) return RuntimeError("Frame must be a tuple", vm);
    if (!CheckMem(vm, 2)) return RuntimeError("Out of memory", vm);
    Env(vm) = ExtendEnv(Env(vm), StackRef(vm, 0), &vm->mem);
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpDefine:
    if (vm->stack.count < 1) return RuntimeError("Stack underflow", vm);
    Define(StackRef(vm, 0), RawInt(ChunkConst(vm->chunk, vm->pc+1)), Env(vm), &vm->mem);
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpLookup:
    if (vm->stack.count < 1) return RuntimeError("Stack underflow", vm);
    if (vm->stack.count + 1 > vm->stack.capacity) return RuntimeError("Stack overflow", vm);
    StackPush(vm, Lookup(RawInt(ChunkConst(vm->chunk, vm->pc+1)), Env(vm), &vm->mem));
    if (StackRef(vm, 0) == Undefined) return RuntimeError("Undefined variable", vm);
    vm->pc += OpLength(op);
    break;
  case OpExport:
    if (vm->stack.count + 1 > vm->stack.capacity) return RuntimeError("Stack overflow", vm);
    StackPush(vm, Head(Env(vm), &vm->mem));
    Env(vm) = Tail(Env(vm), &vm->mem);
    vm->pc += OpLength(op);
    break;
  case OpJump:
    vm->pc += RawInt(ChunkConst(vm->chunk, vm->pc+1));
    break;
  case OpBranch:
    if (vm->stack.count < 1) return RuntimeError("Stack underflow", vm);
    if (StackRef(vm, 0) == Nil || StackRef(vm, 0) == False) {
      vm->pc += OpLength(op);
    } else {
      vm->pc += RawInt(ChunkConst(vm->chunk, vm->pc+1));
    }
    break;
  case OpLink:
    if (vm->stack.count + 2 > vm->stack.capacity) return RuntimeError("Stack overflow", vm);
    StackPush(vm, Env(vm));
    StackPush(vm, IntVal(vm->pc + ChunkConst(vm->chunk, vm->pc+1)));
    vm->pc += OpLength(op);
    break;
  case OpReturn:
    if (vm->stack.count < 3) return RuntimeError("Stack underflow", vm);
    vm->pc = RawInt(StackRef(vm, 1));
    Env(vm) = StackRef(vm, 2);
    StackRef(vm, 2) = StackRef(vm, 0);
    vm->stack.count -= 2;
    break;
  case OpApply:
    if (IsPair(StackRef(vm, 0)) && Head(StackRef(vm, 0), &vm->mem) == Function) {
      /* normal function */
      Val function = StackPop(vm);
      Val num_args = ChunkConst(vm->chunk, vm->pc+1);
      Val arity = ListGet(function, 1, &vm->mem);
      if (arity != Nil && RawInt(num_args) != RawInt(arity)) {
        return RuntimeError("Wrong number of arguments", vm);
      }
      vm->pc = RawInt(ListGet(function, 2, &vm->mem));
      Env(vm) = Tail(Tail(Tail(function, &vm->mem), &vm->mem), &vm->mem);
    } else if (IsPair(StackRef(vm, 0)) && Head(StackRef(vm, 0), &vm->mem) == Primitive) {
      /* apply primitive */
      Val num_args, prim;
      Result result;
      u32 stack_size = vm->stack.count;
      if (stack_size < 3) return RuntimeError("Stack underflow", vm);
      num_args = ChunkConst(vm->chunk, vm->pc+1);
      prim = StackPop(vm);
      result = DoPrimitive(ListGet(prim, 1, &vm->mem), ListGet(prim, 2, &vm->mem), RawInt(num_args), vm);
      if (!result.ok) return result;
      vm->pc = RawInt(StackPop(vm));
      Env(vm) = StackPop(vm);
      StackPush(vm, result.value);
    } else {
      /* not a function, just return */
      if (vm->stack.count < 3) return RuntimeError("Stack underflow", vm);
      vm->pc = RawInt(StackRef(vm, 1));
      Env(vm) = StackRef(vm, 2);
      StackRef(vm, 2) = StackRef(vm, 0);
      vm->stack.count -= 2;
    }
    break;
  default:
    return RuntimeError("Undefined op", vm);
  }

  if (vm->pc < vm->chunk->code.count) {
    return OkResult(True);
  } else {
#ifdef DEBUG
    if (vm->trace) TraceInstruction(OpHalt, vm);
#endif
    return OkResult(False);
  }
}

static bool CheckMem(VM *vm, u32 amount)
{
  if (CapacityLeft(&vm->mem) < amount) {
    CollectGarbage(vm->stack.items, vm->stack.count, &vm->mem);
  }

  if (CapacityLeft(&vm->mem) < amount) {
    ResizeMem(&vm->mem, 2*vm->mem.capacity);
  }

  return CapacityLeft(&vm->mem) >= amount;
}

Result RuntimeError(char *message, VM *vm)
{
  char *filename = ChunkFile(vm->pc, vm->chunk);
  u32 pos = GetSourcePosition(vm->pc, vm->chunk);
  return ErrorResult(message, filename, pos);
}
