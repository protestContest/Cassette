#include "vm.h"
#include "ops.h"
#include "compile/compile.h"
#include "env.h"
#include "primitives.h"
#include "stacktrace.h"
#include "univ/str.h"
#include "univ/math.h"
#include "univ/system.h"

#include "canvas/canvas.h"
#include "debug.h"
#include <stdio.h>

static Result RunInstruction(VM *vm);

void InitVM(VM *vm, Chunk *chunk)
{
  vm->pc = 0;
  vm->link = 0;
  vm->trace = false;
  vm->chunk = chunk;
  vm->dev_map = 0;
  InitVec((Vec*)&vm->stack, sizeof(Val), 256);
  InitMem(&vm->mem, 1024, &vm->stack);

  vm->stack.count = 1;
  Env(vm) = InitialEnv(&vm->mem);
}

void DestroyVM(VM *vm)
{
  u32 i;
  vm->pc = 0;
  vm->chunk = 0;

  for (i = 0; i < ArrayCount(vm->devices); i++) {
    if (vm->dev_map & Bit(i)) {
      DeviceClose(&vm->devices[i], &vm->mem);
    }
  }
  vm->dev_map = 0;

  DestroyVec((Vec*)&vm->stack);
  DestroyMem(&vm->mem);
}

Result Run(VM *vm, u32 num_instructions)
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

  if (vm->trace) PrintTraceHeader();

  while (vm->pc < chunk->code.count && result.ok) {
    result = RunInstruction(vm);
  }

  if (vm->trace) TraceInstruction(OpHalt, vm);

  return result;
}

static void Return(Val value, VM *vm)
{
  vm->link = RawInt(StackPop(vm));
  vm->pc = RawInt(StackPop(vm));
  Env(vm) = StackPop(vm);
  StackPush(vm, value);
}

static Result RunInstruction(VM *vm)
{
  OpCode op = ChunkRef(vm->chunk, vm->pc);

  if (vm->trace) TraceInstruction(op, vm);

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
  case OpConst2: {
    u32 index;
    if (vm->stack.count + 2 > vm->stack.capacity) return RuntimeError("Stack overflow", vm);
    index = (ChunkRef(vm->chunk, vm->pc+1) << 8) | ChunkRef(vm->chunk, vm->pc+2);
    StackPush(vm, vm->chunk->constants.items[index]);
    vm->pc += OpLength(op);
    break;
  }
  case OpInt:
    if (vm->stack.count + 1 > vm->stack.capacity) return RuntimeError("Stack overflow", vm);
    StackPush(vm, IntVal(ChunkRef(vm->chunk, vm->pc+1)));
    vm->pc += OpLength(op);
    break;
  case OpNil:
    if (vm->stack.count + 1 > vm->stack.capacity) return RuntimeError("Stack overflow", vm);
    StackPush(vm, Nil);
    vm->pc += OpLength(op);
    break;
  case OpStr: {
    char *str;
    u32 len;
    if (vm->stack.count < 1) return RuntimeError("Stack underflow", vm);
    if (!IsSym(StackRef(vm, 0))) return RuntimeError("Strings must be made from symbols", vm);

    str = SymbolName(StackRef(vm, 0), &vm->chunk->symbols);
    len = StrLen(str);

    StackRef(vm, 0) = BinaryFrom(str, len, &vm->mem);
    vm->pc += OpLength(op);
    break;
  }
  case OpPair:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    StackRef(vm, 1) = Pair(StackRef(vm, 0), StackRef(vm, 1), &vm->mem);
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpTuple:
    if (vm->stack.count < 1) return RuntimeError("Stack underflow", vm);
    StackRef(vm, 0) = MakeTuple(RawInt(StackRef(vm, 0)), &vm->mem);
    vm->pc += OpLength(op);
    break;
  case OpMap:
    StackPush(vm, MakeMap(&vm->mem));
    vm->pc += OpLength(op);
    break;
  case OpSet:
    if (vm->stack.count < 3) return RuntimeError("Stack underflow", vm);
    if (IsTuple(StackRef(vm, 2), &vm->mem)) {
      if (!IsInt(StackRef(vm, 0))) return RuntimeError("Tuple indexes must be integers", vm);
      if (TupleCount(StackRef(vm, 2), &vm->mem) <= (u32)RawInt(StackRef(vm, 0))) return RuntimeError("Index out of bounds", vm);
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
      if (RawInt(StackRef(vm, 0)) < 0) return RuntimeError("Index out of bounds", vm);
      StackRef(vm, 1) = ListGet(StackRef(vm, 1), RawInt(StackRef(vm, 0)), &vm->mem);
    } else if (IsTuple(StackRef(vm, 1), &vm->mem)) {
      u32 index = RawInt(StackRef(vm, 0));
      Val tuple = StackRef(vm, 1);
      if (!IsInt(StackRef(vm, 0))) return RuntimeError("Index for a tuple must be an integer", vm);
      if (index < 0 || index >= TupleCount(tuple, &vm->mem)) return RuntimeError("Index out of bounds", vm);
      StackRef(vm, 1) = TupleGet(tuple, index, &vm->mem);
    } else if (IsBinary(StackRef(vm, 1), &vm->mem)) {
      u32 index = RawInt(StackRef(vm, 0));
      Val binary = StackRef(vm, 1);
      if (!IsInt(StackRef(vm, 0))) return RuntimeError("Index for a binary must be an integer", vm);
      if (index < 0 || index >= BinaryCount(binary, &vm->mem)) return RuntimeError("Index out of bounds", vm);
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
  case OpExtend:
    if (vm->stack.count < 1) return RuntimeError("Stack underflow", vm);
    if (!IsTuple(StackRef(vm, 0), &vm->mem)) return RuntimeError("Frame must be a tuple", vm);
    Env(vm) = ExtendEnv(Env(vm), StackRef(vm, 0), &vm->mem);
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpDefine:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    Define(StackRef(vm, 1), RawInt(StackRef(vm, 0)), Env(vm), &vm->mem);
    StackPop(vm);
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpLookup:
    if (vm->stack.count < 1) return RuntimeError("Stack underflow", vm);
    if (vm->stack.count + 1 > vm->stack.capacity) return RuntimeError("Stack overflow", vm);
    StackPush(vm, Lookup(RawInt(StackPop(vm)), Env(vm), &vm->mem));
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
    vm->pc += RawInt(StackPop(vm));
    break;
  case OpBranch:
    if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
    if (StackRef(vm, 1) == Nil || StackRef(vm, 1) == False) {
      StackPop(vm);
      vm->pc += OpLength(op);
    } else {
      vm->pc += RawInt(StackPop(vm));
    }
    break;
  case OpLambda:
    if (vm->stack.count < 1) return RuntimeError("Stack underflow", vm);
    if (vm->stack.count + 2 > vm->stack.capacity) return RuntimeError("Stack overflow", vm);
    StackRef(vm, 1) = MakeFunction(StackRef(vm, 1), IntVal(vm->pc + RawInt(StackRef(vm, 0))), Env(vm), &vm->mem);
    StackPop(vm);
    vm->pc += OpLength(op);
    break;
  case OpLink: {
    Val link;
    if (vm->stack.count + 3 > vm->stack.capacity) return RuntimeError("Stack overflow", vm);
    link = StackPop(vm);
    StackPush(vm, Env(vm));
    StackPush(vm, IntVal(vm->pc + RawInt(link)));
    StackPush(vm, IntVal(vm->link));
    vm->link = vm->stack.count;
    vm->pc += OpLength(op);
    break;
  }
  case OpReturn:
    if (vm->stack.count < 4) return RuntimeError("Stack underflow", vm);
    Return(StackPop(vm), vm);
    break;
  case OpApply: {
    Val operator = StackPop(vm);
    i32 num_args = ChunkRef(vm->chunk, vm->pc+1);
    if (IsPrimitive(operator)) {
      /* apply primitive */
      Result result;
      result = DoPrimitive(PrimNum(operator), num_args, vm);
      if (!result.ok) return result;
      StackPush(vm, ResultValue(result));
      vm->pc += OpLength(op);
    } else if (IsFunc(operator, &vm->mem)) {
      /* normal function */
      Val arity = FuncArity(operator, &vm->mem);
      if (arity != Nil && num_args != RawInt(arity)) {
        char msg[255];
        snprintf(msg, 255, "Wrong number of arguments: Expected %d", RawInt(arity));
        return RuntimeError(msg, vm);
      }
      vm->pc = RawInt(FuncPos(operator, &vm->mem));
      Env(vm) = FuncEnv(operator, &vm->mem);
    } else if (IsTuple(operator, &vm->mem) && num_args == 1) {
      Val index = StackPop(vm);
      if (!IsInt(index)) return RuntimeError("Index for a tuple must be an integer", vm);
      if (RawInt(index) < 0 || (u32)RawInt(index) >= TupleCount(operator, &vm->mem)) return RuntimeError("Index out of bounds", vm);
      Return(TupleGet(operator, RawInt(index), &vm->mem), vm);
    } else if (IsMap(operator, &vm->mem) && num_args == 1) {
      Val key = StackPop(vm);
      if (!MapContains(operator, key, &vm->mem)) return RuntimeError("Undefined map key", vm);
      Return(MapGet(operator, key, &vm->mem), vm);
    } else if (IsBinary(operator, &vm->mem) && num_args == 1) {
      Val index = StackPop(vm);
      u8 byte;
      if (!IsInt(index)) return RuntimeError("Index for a binary must be an integer", vm);
      if (RawInt(index) < 0 || (u32)RawInt(index) >= BinaryCount(operator, &vm->mem)) return RuntimeError("Index out of bounds", vm);
      byte = BinaryData(operator, &vm->mem)[RawInt(index)];
      Return(IntVal(byte), vm);
    } else if (IsPair(operator) && num_args == 1) {
      Val index = StackPop(vm);
      if (!IsInt(index)) return RuntimeError("Index for a list must be an integer", vm);
      if (RawInt(index) < 0) return RuntimeError("Index out of bounds", vm);
      Return(ListGet(operator, RawInt(index), &vm->mem), vm);
    } else {
      /* not a function, just return */
      if (num_args != 0) {
        return RuntimeError("Tried to call a non-function", vm);
      }
      if (vm->stack.count < 2) return RuntimeError("Stack underflow", vm);
      Return(operator, vm);
    }
    break;
  }
  default:
    return RuntimeError("Undefined op", vm);
  }

  if (vm->pc < vm->chunk->code.count) {
    return ValueResult(True);
  } else {
    if (vm->trace) TraceInstruction(OpHalt, vm);
    return ValueResult(False);
  }
}

Result RuntimeError(char *message, VM *vm)
{
  char *filename = ChunkFileAt(vm->pc, vm->chunk);
  u32 pos = SourcePosAt(vm->pc, vm->chunk);
  Result result = ErrorResult(message, filename, pos);
  result.data.error->item = StackTrace(vm);
  return result;
}

bool AnyWindowsOpen(VM *vm)
{
  u32 i;
  for (i = 0; i < ArrayCount(vm->devices); i++) {
    if ((vm->dev_map & Bit(i)) && vm->devices[i].type == WindowDevice) {
      return true;
    }
  }
  return false;
}
