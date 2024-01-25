#include "vm.h"
#include "debug.h"
#include "env.h"
#include "ops.h"
#include "primitives.h"
#include "stacktrace.h"
#include "univ/str.h"
#include "univ/math.h"
#include "univ/system.h"

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

#define StackMin(vm, n)     Assert((vm)->stack.count >= (n))
#define CheckStack(vm, n)   if ((vm)->stack.count + (n) > (vm)->stack.capacity)\
return RuntimeError("Stack overflow", vm);

static Result RunInstruction(VM *vm)
{
  OpCode op = ChunkRef(vm->chunk, vm->pc);

  if (vm->trace) TraceInstruction(vm);

  if (ConstOp(op)) {
    u32 index = ((op & 0x7F) << 8) | ChunkRef(vm->chunk, vm->pc+1);
    CheckStack(vm, 1);
    StackPush(vm, ChunkConst(vm->chunk, index));
    vm->pc += 2;
  } else if (IntOp(op)) {
    u32 num = ((op & 0x3F) << 8) | ChunkRef(vm->chunk, vm->pc+1);
    CheckStack(vm, 1);
    StackPush(vm, IntVal(num));
    vm->pc += 2;
  } else {
    switch (op) {
    case OpNoop:
      vm->pc++;
      break;
    case OpHalt:
      Halt(vm);
      break;
    case OpError:
      if (vm->stack.count > 0 && IsSym(StackRef(vm, 0))) {
        char *error = SymbolName(StackRef(vm, 0), &vm->chunk->symbols);
        return RuntimeError(error, vm);
      } else {
        return RuntimeError("Panic!", vm);
      }
      break;
    case OpJump:
      StackMin(vm, 1);
      Assert(IsInt(StackRef(vm, 0)));
      vm->pc += RawInt(StackPop(vm));
      break;
    case OpBranch:
      StackMin(vm, 2);
      if (StackRef(vm, 1) == Nil || StackRef(vm, 1) == False) {
        StackPop(vm);
        vm->pc++;
      } else {
        Assert(IsInt(StackRef(vm, 0)));
        vm->pc += RawInt(StackPop(vm));
      }
      break;
    case OpPop:
      StackMin(vm, 1);
      StackPop(vm);
      vm->pc++;
      break;
    case OpDup:
      StackMin(vm, 1);
      CheckStack(vm, 1);
      StackPush(vm, StackRef(vm, 0));
      vm->pc++;
      break;
    case OpSwap: {
      Val tmp;
      StackMin(vm, 2);
      tmp = StackRef(vm, 0);
      StackRef(vm, 0) = StackRef(vm, 1);
      StackRef(vm, 1) = tmp;
      vm->pc++;
      break;
    }
    case OpDig:
      StackMin(vm, 1);
      Assert(IsInt(StackRef(vm, 0)));
      StackMin(vm, (u32)RawInt(StackRef(vm, 0)));
      StackRef(vm, 0) = StackRef(vm, (u32)RawInt(StackRef(vm, 0)));
      vm->pc++;
      break;
    case OpStr: {
      char *str;
      u32 len;
      StackMin(vm, 1);
      Assert(IsSym(StackRef(vm, 0)));
      str = SymbolName(StackRef(vm, 0), &vm->chunk->symbols);
      Assert(str);
      len = StrLen(str);
      StackRef(vm, 0) = BinaryFrom(str, len, &vm->mem);
      vm->pc++;
      break;
    }
    case OpPair:
      StackMin(vm, 2);
      StackRef(vm, 1) = Pair(StackRef(vm, 0), StackRef(vm, 1), &vm->mem);
      StackPop(vm);
      vm->pc++;
      break;
    case OpNil:
      CheckStack(vm, 1);
      StackPush(vm, Nil);
      vm->pc++;
      break;
    case OpTuple:
      StackMin(vm, 1);
      Assert(IsInt(StackRef(vm, 0)));
      StackRef(vm, 0) = MakeTuple(RawInt(StackRef(vm, 0)), &vm->mem);
      vm->pc++;
      break;
    case OpSet:
      StackMin(vm, 3);
      Assert(IsTuple(StackRef(vm, 2), &vm->mem));
      Assert(IsInt(StackRef(vm, 0)));
      Assert((u32)RawInt(StackRef(vm, 0)) >= 0 &&
        (u32)RawInt(StackRef(vm, 0)) < TupleCount(StackRef(vm, 2), &vm->mem));
      TupleSet(StackRef(vm, 2),
               RawInt(StackRef(vm, 0)),
               StackRef(vm, 1), &vm->mem);
      StackDrop(vm, 2);
      vm->pc++;
      break;
    case OpMap:
      CheckStack(vm, 1);
      StackPush(vm, MakeMap(&vm->mem));
      vm->pc++;
      break;
    case OpPut:
      StackMin(vm, 3);
      Assert(IsMap(StackRef(vm, 2), &vm->mem));
      StackRef(vm, 2) = MapSet(StackRef(vm, 2),
                               StackRef(vm, 0),
                               StackRef(vm, 1), &vm->mem);
      StackDrop(vm, 2);
      vm->pc++;
      break;
    case OpExtend:
      StackMin(vm, 1);
      Assert(IsTuple(StackRef(vm, 0), &vm->mem));
      Env(vm) = Pair(StackRef(vm, 0), Env(vm), &vm->mem);
      StackPop(vm);
      vm->pc++;
      break;
    case OpExport:
      CheckStack(vm, 1);
      StackPush(vm, Head(Env(vm), &vm->mem));
      Env(vm) = Tail(Env(vm), &vm->mem);
      vm->pc++;
      break;
    case OpDefine:
      StackMin(vm, 2);
      Assert(IsInt(StackRef(vm, 0)));
      Define(StackRef(vm, 1), RawInt(StackRef(vm, 0)), Env(vm), &vm->mem);
      StackDrop(vm, 2);
      vm->pc++;
      break;
    case OpLookup:
      StackMin(vm, 1);
      Assert(IsInt(StackRef(vm, 0)));
      StackRef(vm, 0) = Lookup(RawInt(StackRef(vm, 0)), Env(vm), &vm->mem);
      if (StackRef(vm, 0) == Undefined) {
        return RuntimeError("Undefined variable", vm);
      }
      vm->pc++;
      break;
    case OpLink:
      StackMin(vm, 1);
      CheckStack(vm, 2);
      Assert(IsInt(StackRef(vm, 0)));
      StackRef(vm, 0) = IntVal(vm->pc + RawInt(StackRef(vm, 0)));
      StackPush(vm, Env(vm));
      StackPush(vm, IntVal(vm->link));
      vm->link = vm->stack.count;
      vm->pc++;
      break;
    case OpReturn:
      StackMin(vm, 4);
      Assert(IsInt(StackRef(vm, 1)));
      Assert(IsInt(StackRef(vm, 3)));
      vm->link = RawInt(StackRef(vm, 1));
      Env(vm) = StackRef(vm, 2);
      vm->pc = RawInt(StackRef(vm, 3));
      StackRef(vm, 3) = StackRef(vm, 0);
      StackDrop(vm, 3);
      break;
    case OpLambda:
      StackMin(vm, 2);
      Assert(IsInt(StackRef(vm, 0)));
      Assert(IsInt(StackRef(vm, 1)));
      StackRef(vm, 1) = MakeFunction(StackRef(vm, 0),
                                     IntVal(vm->pc + RawInt(StackRef(vm, 1))),
                                     Env(vm),
                                     &vm->mem);
      StackPop(vm);
      vm->pc++;
      break;
    case OpApply: {
      Val operator;
      u32 num_args;

      StackMin(vm, 2);
      Assert(IsInt(StackRef(vm, 0)));
      num_args = RawInt(StackPop(vm));
      operator = StackPop(vm);
      StackMin(vm, num_args);

      if (IsPrimitive(operator)) {
        Result result;
        result = DoPrimitive(PrimNum(operator), num_args, vm);
        if (!result.ok) return result;
        StackDrop(vm, num_args);
        StackPush(vm, ResultValue(result));
        vm->pc++;
      } else if (IsFunc(operator, &vm->mem)) {
        u32 arity = FuncArity(operator, &vm->mem);
        if (num_args != (u32)RawInt(arity)) {
          return RuntimeError("Wrong number of arguments", vm);
        }
        vm->pc = RawInt(FuncPos(operator, &vm->mem));
        Env(vm) = FuncEnv(operator, &vm->mem);
      } else if (num_args == 0) {
        StackMin(vm, 3);
        Assert(IsInt(StackRef(vm, 0)));
        Assert(IsInt(StackRef(vm, 2)));
        vm->link = RawInt(StackPop(vm));
        Env(vm) = StackPop(vm);
        vm->pc = RawInt(StackPop(vm));
        StackPush(vm, operator);
      } else if (num_args == 1 && IsCollection(operator, &vm->mem)) {
        Result result = Access(operator, StackPop(vm), vm);
        if (!result.ok) return result;
        StackMin(vm, 3);
        Assert(IsInt(StackRef(vm, 0)));
        Assert(IsInt(StackRef(vm, 2)));
        vm->link = RawInt(StackPop(vm));
        Env(vm) = StackPop(vm);
        vm->pc = RawInt(StackPop(vm));
        StackPush(vm, ResultValue(result));
      } else {
        return RuntimeError("Tried to call a non-function", vm);
      }
      break;
    }
    default:
      return RuntimeError("Undefined op", vm);
    }
  }

  if (vm->pc < vm->chunk->code.count) {
    return ValueResult(True);
  } else {
    if (vm->trace) TraceInstruction(vm);
    return ValueResult(False);
  }
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
