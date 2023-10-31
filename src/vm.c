#include "vm.h"
#include "env.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  u32 length;
  char *name;
} OpInfo;

static OpInfo ops[NumOps] = {
  [OpNoop]    = {1, "noop"},
  [OpHalt]    = {1, "halt"},
  [OpPop]     = {1, "pop"},
  [OpDup]     = {1, "dup"},
  [OpConst]   = {2, "const"},
  [OpNeg]     = {1, "neg"},
  [OpNot]     = {1, "not"},
  [OpMul]     = {1, "mul"},
  [OpDiv]     = {1, "div"},
  [OpRem]     = {1, "rem"},
  [OpAdd]     = {1, "add"},
  [OpSub]     = {1, "sub"},
  [OpIn]      = {1, "in"},
  [OpLt]      = {1, "lt"},
  [OpGt]      = {1, "gt"},
  [OpEq]      = {1, "eq"},
  [OpStr]     = {1, "str"},
  [OpPair]    = {1, "pair"},
  [OpTuple]   = {2, "tuple"},
  [OpSet]     = {2, "set"},
  [OpGet]     = {1, "get"},
  [OpExtend]  = {1, "extend"},
  [OpDefine]  = {2, "define"},
  [OpLookup]  = {3, "lookup"},
  [OpExport]  = {1, "export"},
  [OpJump]    = {2, "jump"},
  [OpBranch]  = {2, "branch"},
  [OpLink]    = {2, "link"},
  [OpReturn]  = {1, "return"},
  [OpLambda]  = {2, "lambda"},
  [OpApply]   = {1, "apply"}
};

char *OpName(OpCode op)
{
  return ops[op].name;
}

u32 OpLength(OpCode op)
{
  return ops[op].length;
}

static void TraceInstruction(VM *vm, Chunk *chunk);
static void PrintStack(VM *vm, Chunk *chunk);

void InitVM(VM *vm)
{
  vm->pc = 0;
  vm->env = Nil;
  vm->stack.capacity = 1024;
  vm->stack.count = 0;
  vm->stack.data = malloc(sizeof(Val)*1024);
  InitMem(&vm->mem, 1024);
  InitSymbolTable(&vm->symbols);
}

void DestroyVM(VM *vm)
{
  vm->pc = 0;
  vm->stack.capacity = 0;
  vm->stack.count = 0;
  if (vm->stack.data) free(vm->stack.data);
  DestroyMem(&vm->mem);
  DestroySymbolTable(&vm->symbols);
}

char *RunChunk(Chunk *chunk, VM *vm)
{
  while (vm->pc < chunk->count) {
    OpCode op = ChunkRef(chunk, vm->pc);

#ifdef DEBUG
    TraceInstruction(vm, chunk);
#endif

    switch (op) {
    case OpNoop:
      vm->pc += OpLength(op);
      break;
    case OpHalt:
      vm->pc = chunk->count;
      break;
    case OpPop:
      vm->stack.count--;
      vm->pc += OpLength(op);
      break;
    case OpDup:
      vm->stack.count++;
      StackRef(vm, 0) = StackRef(vm, 1);
      vm->pc += OpLength(op);
      break;
    case OpConst:
      vm->stack.count++;
      StackRef(vm, 0) = ChunkConst(chunk, vm->pc+1);
      vm->pc += OpLength(op);
      break;
    case OpNeg:
      if (IsInt(StackRef(vm, 0))) {
        StackRef(vm, 0) = IntVal(-RawInt(StackRef(vm, 0)));
      } else {
        StackRef(vm, 0) = FloatVal(-RawFloat(StackRef(vm, 0)));
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
        StackRef(vm, 0) = ListLength(StackRef(vm, 0), &vm->mem);
      } else if (IsTuple(StackRef(vm, 0), &vm->mem)) {
        StackRef(vm, 0) = TupleLength(StackRef(vm, 0), &vm->mem);
      } else {
        return "Type error";
      }
      vm->pc += OpLength(op);
      break;
    case OpMul:
      if (IsInt(StackRef(vm, 0)) && IsInt(StackRef(vm, 1))) {
        StackRef(vm, 1) = IntVal(RawInt(StackRef(vm, 1)) * RawInt(StackRef(vm, 0)));
      } else if (IsNum(StackRef(vm, 0)) && IsNum(StackRef(vm, 1))) {
        StackRef(vm, 1) = FloatVal(RawNum(StackRef(vm, 1)) * RawNum(StackRef(vm, 0)));
      } else {
        return "Type error";
      }
      vm->stack.count--;
      vm->pc += OpLength(op);
      break;
    case OpDiv:
      if (IsNum(StackRef(vm, 0)) && IsNum(StackRef(vm, 1))) {
        StackRef(vm, 1) = FloatVal((float)RawNum(StackRef(vm, 1)) / (float)RawNum(StackRef(vm, 0)));
      } else {
        return "Type error";
      }
      vm->stack.count--;
      vm->pc += OpLength(op);
      break;
    case OpRem:
      if (IsInt(StackRef(vm, 0)) && IsInt(StackRef(vm, 1))) {
        StackRef(vm, 1) = IntVal(RawInt(StackRef(vm, 1)) % RawInt(StackRef(vm, 0)));
      } else {
        return "Type error";
      }
      vm->stack.count--;
      vm->pc += OpLength(op);
      break;
    case OpAdd:
      if (IsInt(StackRef(vm, 0)) && IsInt(StackRef(vm, 1))) {
        StackRef(vm, 1) = IntVal(RawInt(StackRef(vm, 1)) + RawInt(StackRef(vm, 0)));
      } else if (IsNum(StackRef(vm, 0)) && IsNum(StackRef(vm, 1))) {
        StackRef(vm, 1) = FloatVal(RawNum(StackRef(vm, 1)) + RawNum(StackRef(vm, 0)));
      } else {
        return "Type error";
      }
      vm->stack.count--;
      vm->pc += OpLength(op);
      break;
    case OpSub:
      if (IsInt(StackRef(vm, 0)) && IsInt(StackRef(vm, 1))) {
        StackRef(vm, 1) = IntVal(RawInt(StackRef(vm, 1)) - RawInt(StackRef(vm, 0)));
      } else if (IsNum(StackRef(vm, 0)) && IsNum(StackRef(vm, 1))) {
        StackRef(vm, 1) = FloatVal(RawNum(StackRef(vm, 1)) - RawNum(StackRef(vm, 0)));
      } else {
        return "Type error";
      }
      vm->stack.count--;
      vm->pc += OpLength(op);
      break;
    case OpIn:
      if (IsPair(StackRef(vm, 0))) {
        StackRef(vm, 1) = BoolVal(ListContains(StackRef(vm, 1), StackRef(vm, 0), &vm->mem));
      } else if (IsTuple(StackRef(vm, 0), &vm->mem)) {
        StackRef(vm, 1) = BoolVal(TupleContains(StackRef(vm, 1), StackRef(vm, 0), &vm->mem));
      } else {
        return "Type error";
      }
      vm->stack.count--;
      vm->pc += OpLength(op);
      break;
    case OpLt:
      if (IsNum(StackRef(vm, 0)) && IsNum(StackRef(vm, 1))) {
        StackRef(vm, 1) = BoolVal(RawNum(StackRef(vm, 1)) < RawNum(StackRef(vm, 0)));
      } else {
        return "Type error";
      }
      vm->stack.count--;
      vm->pc += OpLength(op);
      break;
    case OpGt:
      if (IsNum(StackRef(vm, 0)) && IsNum(StackRef(vm, 1))) {
        StackRef(vm, 1) = BoolVal(RawNum(StackRef(vm, 1)) > RawNum(StackRef(vm, 0)));
      } else {
        return "Type error";
      }
      vm->stack.count--;
      vm->pc += OpLength(op);
      break;
    case OpEq:
      StackRef(vm, 1) = BoolVal(StackRef(vm, 1) == StackRef(vm, 0));
      vm->stack.count--;
      vm->pc += OpLength(op);
      break;
    case OpStr:
      if (IsSym(StackRef(vm, 0))) {
        char *str = SymbolName(StackRef(vm, 0), &chunk->symbols);
        StackRef(vm, 0) = BinaryFrom(str, &vm->mem);
      } else {
        return "Type error";
      }
      vm->pc += OpLength(op);
      break;
    case OpPair:
      StackRef(vm, 1) = Pair(StackRef(vm, 1), StackRef(vm, 0), &vm->mem);
      vm->stack.count--;
      vm->pc += OpLength(op);
      break;
    case OpTuple:
      vm->stack.count++;
      StackRef(vm, 0) = MakeTuple(RawInt(ChunkConst(chunk, vm->pc+1)), &vm->mem);
      vm->pc += OpLength(op);
      break;
    case OpSet:
      if (IsTuple(StackRef(vm, 0), &vm->mem)) {
        if (TupleLength(StackRef(vm, 0), &vm->mem) > RawInt(ChunkConst(chunk, vm->pc+1))) {
          TupleSet(StackRef(vm, 0), RawInt(ChunkConst(chunk, vm->pc+1)), StackRef(vm, 1), &vm->mem);
          StackRef(vm, 1) = StackRef(vm, 0);
        } else {
          return "Out of bounds";
        }
        vm->stack.count--;
      } else {
        return "Type error";
      }
      vm->pc += OpLength(op);
      break;
    case OpGet:
      vm->pc += OpLength(op);
      break;
    case OpExtend:
      if (IsTuple(StackRef(vm, 0), &vm->mem)) {
        vm->env = ExtendEnv(vm->env, StackRef(vm, 0), &vm->mem);
      } else {
        return "Type error";
      }
      vm->stack.count--;
      vm->pc += OpLength(op);
      break;
    case OpDefine:
      Define(StackRef(vm, 0), RawInt(ChunkConst(chunk, vm->pc+1)), vm->env, &vm->mem);
      vm->stack.count--;
      vm->pc += OpLength(op);
      break;
    case OpLookup:
      vm->stack.count++;
      StackRef(vm, 0) = Lookup(RawInt(ChunkConst(chunk, vm->pc+1)), RawInt(ChunkConst(chunk, vm->pc+2)), vm->env, &vm->mem);
      if (StackRef(vm, 0) == Undefined) {
        return "Undefined variable";
      }
      vm->pc += OpLength(op);
      break;
    case OpExport:
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
      vm->stack.count += 2;
      StackRef(vm, 1) = vm->env;
      StackRef(vm, 0) = IntVal(vm->pc + ChunkConst(chunk, vm->pc+1));
      vm->pc += OpLength(op);
      break;
    case OpReturn:
      vm->pc = RawInt(StackRef(vm, 1));
      vm->env = StackRef(vm, 2);
      StackRef(vm, 2) = StackRef(vm, 0);
      vm->stack.count -= 2;
      break;
    case OpLambda:
      vm->stack.count++;
      StackRef(vm, 0) = Pair(IntVal(vm->pc + OpLength(op)), vm->env, &vm->mem);
      vm->pc += RawInt(ChunkConst(chunk, vm->pc+1));
      break;
    case OpApply:
      if (IsTrap(StackRef(vm, 0))) {
        return "Unimplemented";
      } else if (IsPair(StackRef(vm, 0))) {
        vm->pc = RawInt(Head(StackRef(vm, 0), &vm->mem));
        vm->env = Tail(StackRef(vm, 0), &vm->mem);
        vm->stack.count--;
      } else {
        return "Type error";
      }
      break;
    default:
      return "Undefined op";
    }

#ifdef DEBUG
    PrintStack(vm, chunk);
    printf("\n");
#endif
  }

  return 0;
}

static void TraceInstruction(VM *vm, Chunk *chunk)
{
  OpCode op = ChunkRef(chunk, vm->pc);
  i32 i, col_width = 20;

  printf("%3d│", vm->pc);
  col_width = 5;
  if (vm->env == Nil) col_width -= printf("   ");
  else col_width -= PrintVal(vm->env, 0);
  for (i = 0; i < col_width; i++) printf(" ");
  printf("│ ");

  col_width = 20;
  col_width -= printf("%s", OpName(op));
  for (i = 0; i < (i32)OpLength(op) - 1; i++) {
    Val arg = ChunkConst(chunk, vm->pc + 1 + i);
    col_width -= printf(" ");

    col_width -= PrintVal(arg, &chunk->symbols);

    if (col_width < 0) break;
  }
  for (i = 0; i < col_width; i++) printf(" ");
  printf(" │");
}

static void PrintStack(VM *vm, Chunk *chunk)
{
  i32 i, col_width = 40;
  for (i = 0; i < (i32)vm->stack.count; i++) {
    col_width -= printf(" ");
    col_width -= PrintVal(StackRef(vm, i), &chunk->symbols);
    if (col_width < 0) break;
  }
}
