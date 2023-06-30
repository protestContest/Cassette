#include "vm.h"
#include "ops.h"

#define StackPush(vm, v)    VecPush((vm)->val, v)
#define StackRef(vm, i)     ((vm)->stack[VecCount((vm)->val) - 1 - (i)])
#define StackPop(vm)        VecPop((vm)->val)

void InitVM(VM *vm)
{
  vm->pc = 0;
  vm->cont = nil;
  vm->env = Pair(MakeValMap(&vm->mem), nil, &vm->mem);
  vm->val = NULL;
  vm->stack = NULL;
  InitMem(&vm->mem);
  vm->chunk = NULL;
}

void MergeStrings(Mem *dst, Mem *src)
{
  for (u32 i = 0; i < MapCount(&src->string_map); i++) {
    u32 key = GetMapKey(&src->string_map, i);
    if (!MapContains(&dst->string_map, key)) {
      char *string = src->strings + MapGet(&src->string_map, key);
      u32 index = VecCount(dst->strings);
      while (*string) {
        VecPush(dst->strings, *string++);
      }
      VecPush(dst->strings, '\0');
      MapSet(&dst->string_map, key, index);
    }
  }
}

void Halt(VM *vm)
{
  vm->pc = VecCount(vm->chunk->data);
}

void RuntimeError(VM *vm, char *message)
{
  Print("Runtime error: ");
  Print(message);
  Print("\n");
  Halt(vm);
}

void Define(Val id, Val value, VM *vm)
{
  Val frame = Head(vm->env, &vm->mem);
  ValMapSet(frame, id, value, &vm->mem);
}

Val Lookup(Val id, VM *vm)
{
  Val env = vm->env;
  Val frame = Head(env, &vm->mem);
  while (!ValMapContains(frame, id, &vm->mem)) {
    env = Tail(env, &vm->mem);
    if (IsNil(env)) {
      RuntimeError(vm, "Undefined variable");
      return nil;
    }
    frame = Head(env, &vm->mem);
  }

  return ValMapGet(frame, id, &vm->mem);
}

void TraceInstruction(VM *vm)
{
  PrintIntN(vm->pc, 3, ' ');
  Print("| ");
  u32 written = PrintInstruction(vm->chunk, vm->pc);

  if (written < 20) {
    for (u32 i = 0; i < 20 - written; i++) Print(" ");
  }

  for (u32 i = 0; i < VecCount(vm->val) && i < 8; i++) {
    Print(" | ");
    DebugVal(vm->val[i], &vm->mem);
  }
  Print("\n");
}

void SaveReg(VM *vm, i32 reg)
{
  if (reg == RegCont) {
    VecPush(vm->stack, vm->cont);
  } else if (reg == RegEnv) {
    VecPush(vm->stack, vm->env);
  } else {
    RuntimeError(vm, "Bad register reference");
  }
}

void RestoreReg(VM *vm, i32 reg)
{
  if (reg == RegCont) {
    vm->cont = VecPop(vm->stack);
  } else if (reg == RegEnv) {
    vm->env = VecPop(vm->stack);
  } else {
    RuntimeError(vm, "Bad register reference");
  }
}

i32 IntOp(i32 a, i32 b, OpCode op)
{
  switch (op) {
  case OpAdd: return a + b;
  case OpSub: return a - b;
  case OpMul: return a * b;
  default: Abort();
  }
}

f32 FloatOp(f32 a, f32 b, OpCode op)
{
  switch (op) {
  case OpAdd: return a + b;
  case OpSub: return a - b;
  case OpMul: return a * b;
  case OpDiv: return a / b;
  default: Abort();
  }
}

void ArithmeticOp(VM *vm, OpCode op)
{
  Val b = StackPop(vm);
  Val a = StackPop(vm);
  Val result;

  if (!IsNumeric(a) || !IsNumeric(b)) {
    RuntimeError(vm, "Bad arithmetic argument");
    return;
  }

  if (IsInt(a) && IsInt(b) && op != OpDiv) {
    result = IntVal(IntOp(RawInt(a), RawInt(b), op));
  } else {
    if (op == OpDiv && IsZero(b)) {
      RuntimeError(vm, "Divide by zero");
      return;
    }

    result = NumVal(FloatOp(RawNum(a), RawNum(b), op));
  }

  StackPush(vm, result);
}

void RunChunk(VM *vm, Chunk *chunk)
{
  vm->pc = 0;
  vm->cont = nil;
  vm->val = NULL;
  vm->stack = NULL;
  vm->chunk = chunk;
  MergeStrings(&vm->mem, &chunk->constants);
  Mem *mem = &vm->mem;

  while (vm->pc < VecCount(chunk->data)) {
    TraceInstruction(vm);

    OpCode op = ChunkRef(chunk, vm->pc);
    switch (op) {
    case OpConst:
      StackPush(vm, ChunkConst(chunk, vm->pc+1));
      vm->pc += OpLength(op);
      break;
    case OpStr:
      StackPush(vm, MakeBinary(SymbolName(StackPop(vm), mem), mem));
      vm->pc += OpLength(op);
      break;
    case OpPair: {
      Val tail = StackPop(vm);
      StackPush(vm, Pair(StackPop(vm), tail, &vm->mem));
      vm->pc += OpLength(op);
      break;
    }
    case OpList: {
      Val list = nil;
      for (u32 i = 0; i < RawInt(ChunkConst(chunk, vm->pc+1)); i++) {
        list = Pair(StackPop(vm), list, mem);
      }
      StackPush(vm, list);
      vm->pc += OpLength(op);
      break;
    }
    case OpTuple: {
      u32 length = RawInt(ChunkConst(chunk, vm->pc+1));
      Val tuple = MakeTuple(length, mem);
      for (u32 i = 0; i < length; i++) {
        TupleSet(tuple, i, StackRef(vm, length - i - 1), mem);
      }
      RewindVec(vm->stack, length);
      StackPush(vm, tuple);
      vm->pc += OpLength(op);
      break;
    }
    case OpMap: {
      Val map = MakeValMap(mem);
      for (u32 i = 0; i < RawInt(ChunkConst(chunk, vm->pc+1)); i++) {
        Val key = StackPop(vm);
        Val value = StackPop(vm);
        ValMapSet(map, key, value, mem);
      }
      StackPush(vm, map);
      vm->pc += OpLength(op);
      break;
    }
    case OpTrue:
      StackPush(vm, SymbolFor("true"));
      vm->pc += OpLength(op);
      break;
    case OpFalse:
      StackPush(vm, SymbolFor("false"));
      vm->pc += OpLength(op);
      break;
    case OpNil:
      StackPush(vm, nil);
      vm->pc += OpLength(op);
      break;
    case OpAdd:
      ArithmeticOp(vm, OpAdd);
      vm->pc += OpLength(op);
      break;
    case OpSub:
      ArithmeticOp(vm, OpSub);
      vm->pc += OpLength(op);
      break;
    case OpMul:
      ArithmeticOp(vm, OpMul);
      vm->pc += OpLength(op);
      break;
    case OpDiv:
      ArithmeticOp(vm, OpDiv);
      vm->pc += OpLength(op);
      break;
    case OpNeg:
      if (!IsNumeric(StackRef(vm, 0))) {
        RuntimeError(vm, "Bad arithmetic argument");
      } else {
        Val n = StackPop(vm);
        if (IsInt(n)) {
          StackPush(vm, IntVal(-RawInt(n)));
        } else {
          StackPush(vm, NumVal(-RawNum(n)));
        }
      }
      vm->pc += OpLength(op);
      break;
    case OpNot:
      if (IsTrue(StackPop(vm))) {
        StackPush(vm, SymbolFor("false"));
      } else {
        StackPush(vm, SymbolFor("true"));
      }
      vm->pc += OpLength(op);
      break;
    case OpEq:
      StackPush(vm, BoolVal(Eq(StackPop(vm), StackPop(vm))));
      vm->pc += OpLength(op);
      break;
    case OpGt: {
      Val b = StackPop(vm);
      Val a = StackPop(vm);
      if (!IsNumeric(a) || !IsNumeric(b)) {
        RuntimeError(vm, "Bad arithmetic argument");
      } else {
        StackPush(vm, BoolVal(RawNum(a) > RawNum(b)));
      }
      vm->pc += OpLength(op);
      break;
    }
    case OpLt: {
      Val b = StackPop(vm);
      Val a = StackPop(vm);
      if (!IsNumeric(a) || !IsNumeric(b)) {
        RuntimeError(vm, "Bad arithmetic argument");
      } else {
        StackPush(vm, BoolVal(RawNum(a) < RawNum(b)));
      }
      vm->pc += OpLength(op);
      break;
    }
    case OpIn: {
      Val collection = StackPop(vm);
      Val item = StackPop(vm);
      if (IsPair(collection)) {
        StackPush(vm, BoolVal(ListContains(collection, item, mem)));
      } else if (IsTuple(collection, mem)) {
        StackPush(vm, BoolVal(TupleContains(collection, item, mem)));
      } else if (IsValMap(collection, mem)) {
        StackPush(vm, BoolVal(ValMapContains(collection, item, mem)));
      } else {
        RuntimeError(vm, "Not a collection");
      }
      vm->pc += OpLength(op);
      break;
    }
    case OpAccess: {
      Val key = StackPop(vm);
      Val map = StackPop(vm);
      if (!ValMapContains(map, key, mem)) {
        RuntimeError(vm, "Undefined key");
      } else {
        StackPush(vm, ValMapGet(map, key, mem));
        vm->pc += OpLength(op);
      }
      break;
    }
    case OpLambda: {
      Val pos = ChunkConst(chunk, vm->pc+1);
      StackPush(vm, Pair(pos, vm->env, mem));
      vm->pc += OpLength(op);
      break;
    }
    case OpSave:
      SaveReg(vm, ChunkRef(chunk, vm->pc+1));
      vm->pc += OpLength(op);
      break;
    case OpRestore:
      RestoreReg(vm, ChunkRef(chunk, vm->pc+1));
      vm->pc += OpLength(op);
      break;
    case OpCont:
      vm->cont = ChunkConst(chunk, vm->pc+1);
      vm->pc += OpLength(op);
      break;
    case OpApply: {
      u32 num_args = ChunkRef(chunk, vm->pc+1);
      Val proc = StackRef(vm, num_args-1);
      vm->pc = RawInt(Head(proc, mem));
      vm->env = Pair(MakeValMap(mem), Tail(proc, mem), mem);
      break;
    }
    case OpReturn:
      vm->pc = RawInt(vm->cont);
      break;
    case OpLookup:
      StackPush(vm, Lookup(StackPop(vm), vm));
      vm->pc += OpLength(op);
      break;
    case OpDefine: {
      Val id = StackPop(vm);
      Val value = StackPop(vm);
      Define(id, value, vm);
      StackPush(vm, value);
      vm->pc += OpLength(op);
      break;
    }
    case OpJump:
      vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      break;
    case OpBranch:
      if (IsTrue(StackRef(vm, 0))) {
        vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      } else {
        vm->pc += OpLength(op);
      }
      break;
    case OpBranchF:
      if (!IsTrue(StackRef(vm, 0))) {
        vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      } else {
        vm->pc += OpLength(op);
      }
      break;
    case OpPop:
      RewindVec(vm->stack, 1);
      vm->pc += OpLength(op);
      break;
    // case OpDefMod: {
    //   Val name = StackPop(vm);
    //   Val frame = Head(vm->env, mem);
    //   ValMapSet(vm->modules, name, frame, mem);
    //   Define(name, frame, vm);
    //   vm->pc += OpLength(op);
    //   break;
    // }
    // case OpImport: {
    //   Val name = StackPop(vm);
    //   if (ValMapContains(vm->modules, name, mem)) {
    //     // cached environment
    //     Define(name, ValMapGet(vm->modules, name, mem), vm);
    //     vm->pc += OpLength(op);
    //   } else if (MapContains(&chunk->modules, name.as_i)) {
    //     // run the module function
    //     vm->pc = MapGet(&chunk->modules, name.as_i);
    //   } else {
    //     RuntimeError(vm, "Module not found");
    //   }
    //   break;
    // }
    case OpHalt:
      vm->pc = VecCount(chunk->data);
      break;
    }
  }

  PrintIntN(vm->pc, 3, ' ');
  Print("| ");
}
