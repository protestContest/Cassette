#include "vm.h"
#include "debug.h"
#include "env.h"
#include "ops.h"
#include "primitives.h"
#include "stacktrace.h"
#include "univ/str.h"
#include "univ/math.h"
#include <stdio.h>

static Result RunInstruction(VM *vm);
static void Return(Val value, VM *vm);

void InitVM(VM *vm, Chunk *chunk)
{
  vm->pc = 0;
  vm->link = 0;
  InitVec(&vm->stack, sizeof(Val), 256);
  InitMem(&vm->mem, 1024, &vm->stack);
  vm->chunk = chunk;
  vm->dev_map = 0;
  vm->trace = false;

  vm->stack.count = 1;
  Env(vm) = InitialEnv(&vm->mem);
}

void DestroyVM(VM *vm)
{
  u32 i;
  vm->pc = 0;
  vm->link = 0;
  DestroyVec(&vm->stack);
  DestroyMem(&vm->mem);
  vm->chunk = 0;

  for (i = 0; i < ArrayCount(vm->devices); i++) {
    if (vm->dev_map & Bit(i)) {
      DeviceClose(&vm->devices[i], &vm->mem);
    }
  }
  vm->dev_map = 0;
}

Result Run(u32 num_instructions, VM *vm)
{
  u32 i;
  Result result = ValueResult(BoolVal(vm->pc < vm->chunk->code.count));
  if (!vm->chunk) return RuntimeError("VM not initialized", vm);

  for (i = 0; i < num_instructions && ResultValue(result) == True; i++) {
    result = RunInstruction(vm);
    if (!result.ok) return result;
  }

  return result;
}

Result RunChunk(Chunk *chunk, VM *vm)
{
  Result result = ValueResult(Nil);
  vm->pc = 0;
  vm->chunk = chunk;

  if (vm->trace) PrintTraceHeader(chunk->code.count);

  while (result.ok) result = RunInstruction(vm);

  return result;
}

#define StackMin(vm, n)   if ((vm)->stack.count < (n)) return RuntimeError("Stack underflow", vm);
#define CheckStack(vm, n)   if ((vm)->stack.count + (n) > (vm)->stack.capacity) return RuntimeError("Stack overflow", vm);

static Result RunInstruction(VM *vm)
{
  OpCode op = ChunkRef(vm->chunk, vm->pc);

  if (vm->trace) TraceInstruction(vm);

  switch (op) {
  case OpNoop:
    vm->pc += OpLength(op);
    break;
  case OpHalt:
    Halt(vm);
    break;
  case OpError:
    if (vm->stack.count > 0 && IsSym(StackRef(vm, 0))) {
      return RuntimeError(SymbolName(StackRef(vm, 0), &vm->chunk->symbols), vm);
    } else {
      return RuntimeError("Panic!", vm);
    }
    break;
  case OpPop:
    StackMin(vm, 1);
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpDup:
    StackMin(vm, 1);
    CheckStack(vm, 1);
    StackPush(vm, StackRef(vm, 0));
    vm->pc += OpLength(op);
    break;
  case OpSwap: {
    Val tmp;
    StackMin(vm, 2);
    tmp = StackRef(vm, 0);
    StackRef(vm, 0) = StackRef(vm, 1);
    StackRef(vm, 1) = tmp;
    vm->pc += OpLength(op);
    break;
  }
  case OpConst:
    CheckStack(vm, 1);
    StackPush(vm, ChunkConst(vm->chunk, vm->pc+1));
    vm->pc += OpLength(op);
    break;
  case OpConst2: {
    u32 index;
    CheckStack(vm, 1);
    index = (ChunkRef(vm->chunk, vm->pc+1) << 8) | ChunkRef(vm->chunk, vm->pc+2);
    StackPush(vm, vm->chunk->constants.items[index]);
    vm->pc += OpLength(op);
    break;
  }
  case OpInt:
    CheckStack(vm, 1);
    StackPush(vm, IntVal(ChunkRef(vm->chunk, vm->pc+1)));
    vm->pc += OpLength(op);
    break;
  case OpNil:
    CheckStack(vm, 1);
    StackPush(vm, Nil);
    vm->pc += OpLength(op);
    break;
  case OpStr: {
    char *str;
    u32 len;
    StackMin(vm, 1);
    if (!IsSym(StackRef(vm, 0))) return RuntimeError("Strings must be made from symbols", vm);

    str = SymbolName(StackRef(vm, 0), &vm->chunk->symbols);
    len = StrLen(str);

    StackRef(vm, 0) = BinaryFrom(str, len, &vm->mem);
    vm->pc += OpLength(op);
    break;
  }
  case OpPair:
    StackMin(vm, 2);
    StackRef(vm, 1) = Pair(StackRef(vm, 0), StackRef(vm, 1), &vm->mem);
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpTuple:
    StackMin(vm, 1);
    StackRef(vm, 0) = MakeTuple(RawInt(StackRef(vm, 0)), &vm->mem);
    vm->pc += OpLength(op);
    break;
  case OpSet:
    StackMin(vm, 3);
    if (!IsTuple(StackRef(vm, 2), &vm->mem)) return RuntimeError("Tuple set is only defined for tuples", vm);
    if (!IsInt(StackRef(vm, 0))) return RuntimeError("Tuple indexes must be integers", vm);
    if (TupleCount(StackRef(vm, 2), &vm->mem) <= (u32)RawInt(StackRef(vm, 0))) return RuntimeError("Index out of bounds", vm);

    TupleSet(StackRef(vm, 2), RawInt(StackRef(vm, 0)), StackRef(vm, 1), &vm->mem);
    StackPop(vm);
    StackPop(vm);

    vm->pc += OpLength(op);
    break;
  case OpMap:
    StackPush(vm, MakeMap(&vm->mem));
    vm->pc += OpLength(op);
    break;
  case OpPut:
    StackMin(vm, 3);
    if (!IsMap(StackRef(vm, 2), &vm->mem)) return RuntimeError("Map put is only defined for maps", vm);

    StackRef(vm, 2) = MapSet(StackRef(vm, 2), StackRef(vm, 0), StackRef(vm, 1), &vm->mem);
    StackPop(vm);
    StackPop(vm);

    vm->pc += OpLength(op);
    break;
  case OpExtend:
    StackMin(vm, 1);
    if (!IsTuple(StackRef(vm, 0), &vm->mem)) return RuntimeError("Frame must be a tuple", vm);
    Env(vm) = ExtendEnv(Env(vm), StackRef(vm, 0), &vm->mem);
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpDefine:
    StackMin(vm, 2);
    Define(StackRef(vm, 1), RawInt(StackRef(vm, 0)), Env(vm), &vm->mem);
    StackPop(vm);
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpLookup:
    StackMin(vm, 1);
    if (!IsInt(StackRef(vm, 0))) return RuntimeError("Lookups must be integers", vm);
    StackRef(vm, 0) = Lookup(RawInt(StackRef(vm, 0)), Env(vm), &vm->mem);
    if (StackRef(vm, 0) == Undefined) return RuntimeError("Undefined variable", vm);
    vm->pc += OpLength(op);
    break;
  case OpExport:
    CheckStack(vm, 1);
    StackPush(vm, Head(Env(vm), &vm->mem));
    Env(vm) = Tail(Env(vm), &vm->mem);
    vm->pc += OpLength(op);
    break;
  case OpJump:
    StackMin(vm, 1);
    if (!IsInt(StackRef(vm, 0))) return RuntimeError("Attempted jump with a non-integer", vm);
    vm->pc += RawInt(StackPop(vm));
    break;
  case OpBranch:
    StackMin(vm, 2);
    if (StackRef(vm, 1) == Nil || StackRef(vm, 1) == False) {
      StackPop(vm);
      vm->pc += OpLength(op);
    } else {
      if (!IsInt(StackRef(vm, 0))) return RuntimeError("Attempted branch with a non-integer", vm);
      vm->pc += RawInt(StackPop(vm));
    }
    break;
  case OpLambda:
    StackMin(vm, 2);
    StackRef(vm, 1) = MakeFunction(StackRef(vm, 1), IntVal(vm->pc + RawInt(StackRef(vm, 0))), Env(vm), &vm->mem);
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpLink: {
    Val link;
    StackMin(vm, 1);
    CheckStack(vm, 2);
    link = StackPop(vm);
    StackPush(vm, Env(vm));
    StackPush(vm, IntVal(vm->pc + RawInt(link)));
    StackPush(vm, IntVal(vm->link));
    vm->link = vm->stack.count;
    vm->pc += OpLength(op);
    break;
  }
  case OpReturn:
    StackMin(vm, 4);
    Return(StackPop(vm), vm);
    break;
  case OpApply: {
    Val operator;
    u32 num_args;

    StackMin(vm, 1);
    operator = StackPop(vm);
    num_args = ChunkRef(vm->chunk, vm->pc+1);
    StackMin(vm, num_args);

    if (IsPrimitive(operator)) {
      Result result;
      result = DoPrimitive(PrimNum(operator), num_args, vm);
      if (!result.ok) return result;

      StackPush(vm, ResultValue(result));
      vm->pc += OpLength(op);
    } else if (IsFunc(operator, &vm->mem)) {
      Val arity = FuncArity(operator, &vm->mem);
      if (arity != Nil && num_args != (u32)RawInt(arity)) {
        char msg[255];
        snprintf(msg, 255, "Wrong number of arguments: Expected %d", RawInt(arity));
        return RuntimeError(msg, vm);
      }

      vm->pc = RawInt(FuncPos(operator, &vm->mem));
      Env(vm) = FuncEnv(operator, &vm->mem);
    } else if (num_args == 0) {
      StackMin(vm, 3);
      Return(operator, vm);
    } else if (num_args == 1) {
      if (IsTuple(operator, &vm->mem)) {
        Val index = StackPop(vm);
        if (!IsInt(index)) return RuntimeError("Index for a tuple must be an integer", vm);
        if (RawInt(index) < 0 || (u32)RawInt(index) >= TupleCount(operator, &vm->mem)) return RuntimeError("Index out of bounds", vm);

        StackMin(vm, 3);
        Return(TupleGet(operator, RawInt(index), &vm->mem), vm);
      } else if (IsMap(operator, &vm->mem) && num_args == 1) {
        Val key = StackPop(vm);
        if (!MapContains(operator, key, &vm->mem)) return RuntimeError("Undefined map key", vm);

        StackMin(vm, 3);
        Return(MapGet(operator, key, &vm->mem), vm);
      } else if (IsBinary(operator, &vm->mem) && num_args == 1) {
        Val index = StackPop(vm);
        u8 byte;
        if (!IsInt(index)) return RuntimeError("Index for a binary must be an integer", vm);
        if (RawInt(index) < 0 || (u32)RawInt(index) >= BinaryCount(operator, &vm->mem)) return RuntimeError("Index out of bounds", vm);

        byte = BinaryData(operator, &vm->mem)[RawInt(index)];
        StackMin(vm, 3);
        Return(IntVal(byte), vm);
      } else if (IsPair(operator) && num_args == 1) {
        Val index = StackPop(vm);
        if (!IsInt(index)) return RuntimeError("Index for a list must be an integer", vm);
        if (RawInt(index) < 0) return RuntimeError("Index out of bounds", vm);
        if (vm->stack.count < 3) return RuntimeError("Stack underflow", vm);

        Return(ListGet(operator, RawInt(index), &vm->mem), vm);
      } else {
        return RuntimeError("Tried to call a non-function", vm);
      }
    } else {
      return RuntimeError("Tried to call a non-function", vm);
    }
    break;
  }
  default:
    return RuntimeError("Undefined op", vm);
  }

  if (vm->pc < vm->chunk->code.count) {
    return ValueResult(True);
  } else {
    if (vm->trace) TraceInstruction(vm);
    return ValueResult(False);
  }
}

static void Return(Val value, VM *vm)
{
  vm->link = RawInt(StackPop(vm));
  vm->pc = RawInt(StackPop(vm));
  Env(vm) = StackPop(vm);
  StackPush(vm, value);
}

void Halt(VM *vm)
{
  vm->pc = vm->chunk->code.count;
}

Result RuntimeError(char *message, VM *vm)
{
  char *filename = ChunkFileAt(vm->pc, vm->chunk);
  u32 pos = SourcePosAt(vm->pc, vm->chunk);
  Result result = ErrorResult(message, filename, pos);
  result.data.error->item = StackTrace(vm);
  return result;
}
