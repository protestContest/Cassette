#include "vm.h"
#include "ops.h"
#include "compile/compile.h"
#include "env.h"
#include "primitives.h"
#include "univ/string.h"
#include <stdio.h>
#include <stdlib.h>

static bool CheckMem(VM *vm, u32 amount);

#ifdef DEBUG
static void PrintTraceHeader(void);
static void TraceInstruction(OpCode op, VM *vm);
#endif

void InitVM(VM *vm)
{
  vm->pc = 0;
  vm->chunk = 0;
  InitVec((Vec*)&vm->stack, sizeof(Val), 256);
  InitMem(&vm->mem, 1024*1024);

  vm->stack.count = 1;
  Env(vm) = ExtendEnv(Nil, DefinePrimitives(&vm->mem, 0), &vm->mem);
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

#ifdef DEBUG
  PrintTraceHeader();
#endif

  while (vm->pc < chunk->code.count) {
    OpCode op = ChunkRef(chunk, vm->pc);

#ifdef DEBUG
    TraceInstruction(op, vm);
    printf("\n");
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
        return RuntimeError("Type error", vm);
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
        return RuntimeError("Type error", vm);
      }
      vm->pc += OpLength(op);
      break;
    case OpMul:
      if (IsInt(StackRef(vm, 0)) && IsInt(StackRef(vm, 1))) {
        StackRef(vm, 1) = IntVal(RawInt(StackRef(vm, 1)) * RawInt(StackRef(vm, 0)));
      } else if (IsNum(StackRef(vm, 0)) && IsNum(StackRef(vm, 1))) {
        StackRef(vm, 1) = FloatVal(RawNum(StackRef(vm, 1)) * RawNum(StackRef(vm, 0)));
      } else {
        return RuntimeError("Type error", vm);
      }
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpDiv:
      if (IsNum(StackRef(vm, 0)) && IsNum(StackRef(vm, 1))) {
        if ((float)RawNum(StackRef(vm, 0)) == 0.0) return RuntimeError("Divide by zero", vm);
        StackRef(vm, 1) = FloatVal((float)RawNum(StackRef(vm, 1)) / (float)RawNum(StackRef(vm, 0)));
      } else {
        return RuntimeError("Type error", vm);
      }
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpRem:
      if (IsInt(StackRef(vm, 0)) && IsInt(StackRef(vm, 1))) {
        StackRef(vm, 1) = IntVal(RawInt(StackRef(vm, 1)) % RawInt(StackRef(vm, 0)));
      } else {
        return RuntimeError("Type error", vm);
      }
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpAdd:
      if (IsInt(StackRef(vm, 0)) && IsInt(StackRef(vm, 1))) {
        StackRef(vm, 1) = IntVal(RawInt(StackRef(vm, 1)) + RawInt(StackRef(vm, 0)));
      } else if (IsNum(StackRef(vm, 0)) && IsNum(StackRef(vm, 1))) {
        StackRef(vm, 1) = FloatVal(RawNum(StackRef(vm, 1)) + RawNum(StackRef(vm, 0)));
      } else {
        return RuntimeError("Type error", vm);
      }
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpSub:
      if (IsInt(StackRef(vm, 0)) && IsInt(StackRef(vm, 1))) {
        StackRef(vm, 1) = IntVal(RawInt(StackRef(vm, 1)) - RawInt(StackRef(vm, 0)));
      } else if (IsNum(StackRef(vm, 0)) && IsNum(StackRef(vm, 1))) {
        StackRef(vm, 1) = FloatVal(RawNum(StackRef(vm, 1)) - RawNum(StackRef(vm, 0)));
      } else {
        return RuntimeError("Type error", vm);
      }
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpIn:
      if (IsPair(StackRef(vm, 0))) {
        StackRef(vm, 1) = BoolVal(ListContains(StackRef(vm, 1), StackRef(vm, 0), &vm->mem));
      } else if (IsTuple(StackRef(vm, 0), &vm->mem)) {
        StackRef(vm, 1) = BoolVal(TupleContains(StackRef(vm, 1), StackRef(vm, 0), &vm->mem));
      } else {
        return RuntimeError("Type error", vm);
      }
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpLt:
      if (IsNum(StackRef(vm, 0)) && IsNum(StackRef(vm, 1))) {
        StackRef(vm, 1) = BoolVal(RawNum(StackRef(vm, 1)) < RawNum(StackRef(vm, 0)));
      } else {
        return RuntimeError("Type error", vm);
      }
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpGt:
      if (IsNum(StackRef(vm, 0)) && IsNum(StackRef(vm, 1))) {
        StackRef(vm, 1) = BoolVal(RawNum(StackRef(vm, 1)) > RawNum(StackRef(vm, 0)));
      } else {
        return RuntimeError("Type error", vm);
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
      if (!IsSym(StackRef(vm, 0))) return RuntimeError("Type error", vm);

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
      if (!IsTuple(StackRef(vm, 0), &vm->mem)) return RuntimeError("Type error", vm);
      if (TupleLength(StackRef(vm, 0), &vm->mem) <= (u32)RawInt(ChunkConst(chunk, vm->pc+1))) return RuntimeError("Out of bounds", vm);

      TupleSet(StackRef(vm, 0), RawInt(ChunkConst(chunk, vm->pc+1)), StackRef(vm, 1), &vm->mem);
      StackRef(vm, 1) = StackRef(vm, 0);
      StackPop(vm);

      vm->pc += OpLength(op);
      break;
    case OpGet:
      if (IsPair(StackRef(vm, 0))) {
        u32 index = RawInt(ChunkConst(chunk, vm->pc+1));
        Val list = StackRef(vm, 1);
        while (index > 0) {
          if (list == Nil) return RuntimeError("Out of bounds", vm);
          list = Tail(list, &vm->mem);
          index--;
        }
        StackPush(vm, Head(list, &vm->mem));
      } else if (IsTuple(StackRef(vm, 0), &vm->mem)) {
        u32 index = RawInt(ChunkConst(chunk, vm->pc+1));
        Val tuple = StackRef(vm, 0);
        if (index >= TupleLength(tuple, &vm->mem)) return RuntimeError("Out of bounds", vm);
        StackPush(vm, TupleGet(tuple, index, &vm->mem));
      } else {
        return RuntimeError("Type error", vm);
      }
      vm->pc += OpLength(op);
      break;
    case OpExtend:
      if (!IsTuple(StackRef(vm, 0), &vm->mem)) return RuntimeError("Type error", vm);
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
      if (IsPair(StackRef(vm, 0))) {
        Val num_args = ChunkConst(chunk, vm->pc+1);
        if (Head(StackRef(vm, 0), &vm->mem) == Primitive) {
          /* apply primitive */
          Val prim = StackPop(vm);
          Result result = DoPrimitive(Tail(prim, &vm->mem), RawInt(num_args), vm);
          if (!result.ok) return result;
          vm->pc = RawInt(StackPop(vm));
          Env(vm) = StackPop(vm);
          StackPush(vm, result.value);
        } else {
          /* normal function */
          vm->pc = RawInt(Head(StackRef(vm, 0), &vm->mem));
          Env(vm) = Tail(StackRef(vm, 0), &vm->mem);
          StackRef(vm, 0) = num_args;
        }
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
    printf("\n");
#endif

  return OkResult(Nil);
}

static bool CheckMem(VM *vm, u32 amount)
{
  if (!CheckCapacity(&vm->mem, amount)) {
#ifdef DEBUG
    printf("Collecting Garbage\n");
#endif
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

#ifdef DEBUG
static void PrintTraceHeader(void)
{
  printf(" PC   Instruction            Stack\n");
  printf("────┬──────────────────────┬───────────────────────\n");
}

static void TraceInstruction(OpCode op, VM *vm)
{
  i32 i, col_width;

  printf("%4d│ ", vm->pc);

  col_width = 20;
  col_width -= printf("%s", OpName(op));
  for (i = 0; i < (i32)OpLength(op) - 1; i++) {
    Val arg = ChunkConst(vm->chunk, vm->pc + 1 + i);
    col_width -= printf(" ");

    col_width -= PrintVal(arg, &vm->chunk->symbols);

    if (col_width < 0) break;
  }
  for (i = 0; i < col_width; i++) printf(" ");
  printf(" │");

  col_width = 80;
  for (i = 0; i < (i32)vm->stack.count; i++) {
    col_width -= printf(" ");
    col_width -= PrintVal(StackRef(vm, i), &vm->chunk->symbols);
    if (col_width < 0) {
      printf(" ...");
      break;
    }
  }
}
#endif
