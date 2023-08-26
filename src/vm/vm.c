#include "vm.h"
#include "ops.h"
#include "env.h"
#include "function.h"
#include "lib.h"
#include "debug.h"

void InitVM(VM *vm, Heap *mem)
{
  vm->pc = 0;
  vm->cont = 0;
  vm->stack = NULL;
  vm->call_stack = NULL;
  vm->env = InitialEnv(mem);
  vm->modules = nil;
  vm->mem = mem;
  vm->error = false;
  vm->verbose = false;

  MakeSymbol("ok", mem);
  MakeSymbol("error", mem);
}

void DestroyVM(VM *vm)
{
  FreeVec(vm->stack);
}

char *VMErrorMessage(VMError err)
{
  switch (err) {
  case NoError:         return "  NoError";
  case StackError:      return "  StackError";
  case TypeError:       return "  TypeError";
  case ArithmeticError: return "  ArithmeticError";
  case EnvError:        return "  EnvError";
  case KeyError:        return "  KeyError";
  }
}

void StackPush(VM *vm, Val value)
{
  VecPush(vm->stack, value);
}

Val StackPop(VM *vm)
{
  if (VecCount(vm->stack) == 0) {
    vm->error = StackError;
    return nil;
  }

  return VecPop(vm->stack);
}

#define StackSize(vm)     VecCount((vm)->stack)
#define StackPeek(vm, n)  VecPeek((vm)->stack, n)

static void CallStackPush(VM *vm, Val value)
{
  VecPush(vm->call_stack, value);
}

static Val CallStackPop(VM *vm)
{
  if (VecCount(vm->call_stack) == 0) {
    vm->error = StackError;
    return nil;
  }

  return VecPop(vm->call_stack);
}

void RuntimeError(VM *vm, VMError error)
{
  vm->error = error;
}

static u32 RunGC(VM *vm)
{
  StackPush(vm, vm->env);
  CollectGarbage(vm->stack, vm->mem);
  vm->env = StackPop(vm);
  return MemSize(vm->mem);
}

#define GCFreq  1024
void RunChunk(VM *vm, Chunk *chunk)
{
  Heap *mem = vm->mem;
  u32 gc = GCFreq;

  vm->pc = 0;
  vm->cont = ChunkSize(chunk);
  vm->stack = NULL;
  vm->modules = MakeTuple(chunk->num_modules, mem);
  vm->error = false;

  CopyStrings(&chunk->constants, mem);

  while (vm->error == NoError && vm->pc < ChunkSize(chunk)) {
    if (vm->verbose) {
      TraceInstruction(vm, chunk);
    }

    if (--gc == 0) {
      gc = GCFreq;
      RunGC(vm);
    }

    OpCode op = ChunkRef(chunk, vm->pc);
    switch (op) {
    case OpHalt:
      vm->pc = ChunkSize(chunk);
      break;
    case OpPop:
      StackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpDup:
      StackPush(vm, StackPeek(vm, 0));
      vm->pc += OpLength(op);
      break;
    case OpConst:
      StackPush(vm, ChunkConst(chunk, vm->pc+1));
      vm->pc += OpLength(op);
      break;
    case OpGet: {
      Val key = StackPop(vm);
      Val obj = StackPop(vm);
      if (IsPair(obj)) {
        if (IsInt(key) && RawInt(key) >= 0) {
          StackPush(vm, ListAt(obj, RawInt(key), mem));
        } else {
          vm->error = KeyError;
        }
      } else if (IsTuple(obj, mem)) {
        if (IsInt(key) && RawInt(key) >= 0
            && RawInt(key) < TupleLength(obj, mem)) {
          StackPush(vm, TupleGet(obj, RawInt(key), mem));
        } else {
          vm->error = KeyError;
        }
      } else if (IsMap(obj, mem)) {
        if (MapContains(obj, key, mem)) {
          StackPush(vm, MapGet(obj, key, mem));
        } else {
          vm->error = KeyError;
        }
      } else {
        vm->error = TypeError;
      }
      vm->pc += OpLength(op);
      break;
    }
    case OpNeg: {
      Val val = StackPop(vm);
      if (IsInt(val)) {
        StackPush(vm, IntVal(-RawInt(val)));
      } else if (IsFloat(val)) {
        StackPush(vm, FloatVal(-RawFloat(val)));
      } else {
        vm->error = TypeError;
      }
      vm->pc += OpLength(op);
      break;
    }
    case OpNot:
      StackPush(vm, BoolVal(IsTrue(StackPop(vm))));
      vm->pc += OpLength(op);
      break;
    case OpLen: {
      Val val = StackPop(vm);
      if (IsPair(val)) {
        StackPush(vm, IntVal(ListLength(val, mem)));
      } else if (IsTuple(val, mem)) {
        StackPush(vm, IntVal(TupleLength(val, mem)));
      } else {
        vm->error = TypeError;
      }
      vm->pc += OpLength(op);
      break;
    }
    case OpMul: {
      Val a = StackPop(vm);
      Val b = StackPop(vm);
      if (IsInt(a) && IsInt(b)) {
        StackPush(vm, IntVal(RawInt(b) * RawInt(a)));
      } else if (IsNum(b) && IsNum(a)) {
        StackPush(vm, FloatVal(RawNum(b) * RawNum(a)));
      } else {
        vm->error = TypeError;
      }
      vm->pc += OpLength(op);
      break;
    }
    case OpDiv: {
      Val a = StackPop(vm);
      Val b = StackPop(vm);
      if (IsNum(a) && RawNum(a) == 0) {
        vm->error = ArithmeticError;
      } else if (IsNum(b) && IsNum(a)) {
        StackPush(vm, FloatVal((float)RawNum(b) / (float)RawNum(a)));
      } else {
        vm->error = TypeError;
      }
      vm->pc += OpLength(op);
      break;
    }
    case OpRem: {
      Val a = StackPop(vm);
      Val b = StackPop(vm);
      if (IsNum(a) && RawNum(a) == 0) {
        vm->error = ArithmeticError;
      } else if (IsInt(a) && IsInt(b)) {
        StackPush(vm, IntVal(RawInt(b) % RawInt(a)));
      } else {
        vm->error = TypeError;
      }
      vm->pc += OpLength(op);
      break;
    }
    case OpAdd: {
      Val a = StackPop(vm);
      Val b = StackPop(vm);
      if (IsInt(a) && IsInt(b)) {
        StackPush(vm, IntVal(RawInt(a) + RawInt(b)));
      } else if (IsNum(a) && IsNum(b)) {
        StackPush(vm, FloatVal(RawNum(a) + RawNum(b)));
      } else {
        vm->error = TypeError;
      }
      vm->pc += OpLength(op);
      break;
    }
    case OpSub:{
      Val a = StackPop(vm);
      Val b = StackPop(vm);
      if (IsInt(a) && IsInt(b)) {
        StackPush(vm, IntVal(RawInt(b) - RawInt(a)));
      } else if (IsNum(a) && IsNum(b)) {
        StackPush(vm, FloatVal(RawNum(b) - RawNum(a)));
      } else {
        vm->error = TypeError;
      }
      vm->pc += OpLength(op);
      break;
    }
    case OpIn: {
      Val obj = StackPop(vm);
      Val item = StackPop(vm);
      if (IsPair(obj)) {
        StackPush(vm, BoolVal(ListContains(obj, item, mem)));
      } else if (IsTuple(obj, mem)) {
        StackPush(vm, BoolVal(TupleContains(obj, item, mem)));
      } else if (IsMap(obj, mem)) {
        StackPush(vm, BoolVal(MapContains(obj, item, mem)));
      } else {
        vm->error = TypeError;
      }
      vm->pc += OpLength(op);
      break;
    }
    case OpGt: {
      Val a = StackPop(vm);
      Val b = StackPop(vm);
      if (IsNum(a) && IsNum(b)) {
        StackPush(vm, BoolVal(RawNum(b) > RawNum(a)));
      } else {
        vm->error = TypeError;
      }
      vm->pc += OpLength(op);
      break;
    }
    case OpLt: {
      Val a = StackPop(vm);
      Val b = StackPop(vm);
      if (IsNum(a) && IsNum(b)) {
        StackPush(vm, BoolVal(RawNum(b) < RawNum(a)));
      } else {
        vm->error = TypeError;
      }
      vm->pc += OpLength(op);
      break;
    }
    case OpEq:
      StackPush(vm, BoolVal(Eq(StackPop(vm), StackPop(vm))));
      vm->pc += OpLength(op);
      break;
    case OpPair: {
      Val head = StackPop(vm);
      Val tail = StackPop(vm);
      StackPush(vm, Pair(head, tail, mem));
      vm->pc += OpLength(op);
      break;
    }
    case OpTuple: {
      u32 len = RawInt(ChunkConst(chunk, vm->pc+1));
      Val tuple = MakeTuple(len, mem);
      StackPush(vm, tuple);
      vm->pc += OpLength(op);
      break;
    }
    case OpSet: {
      Val item = StackPop(vm);
      u32 index = RawInt(ChunkConst(chunk, vm->pc+1));
      if (IsTuple(StackPeek(vm, 0), mem)) {
        TupleSet(StackPeek(vm, 0), index, item, mem);
      } else {
        vm->error = TypeError;
      }
      vm->pc += OpLength(op);
      break;
    }
    case OpStr: {
      Val sym = StackPop(vm);
      if (!IsSym(sym)) {
        vm->error = TypeError;
      } else {
        char *name = SymbolName(sym, mem);
        StackPush(vm, BinaryFrom(name, StrLen(name), mem));
      }
      vm->pc += OpLength(op);
      break;
    }
    case OpMap: {
      Val keys = StackPop(vm);
      Val vals = StackPop(vm);
      if (!IsTuple(keys, mem) || !IsTuple(vals, mem)
          || TupleLength(keys, mem) != TupleLength(vals, mem)) {
        vm->error = TypeError;
      } else {
        Val map = MapFrom(keys, vals, mem);
        StackPush(vm, map);
      }
      vm->pc += OpLength(op);
      break;
    }
    case OpLambda: {
      u32 size = RawInt(ChunkConst(chunk, vm->pc+1));
      StackPush(vm, MakeFunc(IntVal(vm->pc+2), vm->env, mem));
      vm->pc += OpLength(op) + size;
      break;
    }
    case OpExtend: {
      Val frame = StackPop(vm);
      vm->env = ExtendEnv(vm->env, frame, mem);
      vm->pc += OpLength(op);
      break;
    }
    case OpDefine:
      Define(RawInt(ChunkConst(chunk, vm->pc+1)), StackPop(vm), vm->env, mem);
      vm->pc += OpLength(op);
      break;
    case OpLookup: {
      u32 frame = RawInt(ChunkConst(chunk, vm->pc+1));
      u32 pos = RawInt(ChunkConst(chunk, vm->pc+2));
      Val value = Lookup(vm->env, frame, pos, mem);
      if (Eq(value, SymbolFor("*undefined*"))) {
        vm->error = EnvError;
      } else {
        StackPush(vm, value);
      }
      vm->pc += OpLength(op);
      break;
    }
    case OpExport:
      StackPush(vm, Head(vm->env, mem));
      vm->pc += OpLength(op);
      break;
    case OpJump:
      vm->pc += OpLength(op) + RawInt(ChunkConst(chunk, vm->pc+1));
      break;
    case OpBranch:
      if (IsTrue(StackPeek(vm, 0))) {
        vm->pc += OpLength(op) + RawInt(ChunkConst(chunk, vm->pc+1));
      } else {
        vm->pc += OpLength(op);
      }
      break;
    case OpCont:
      vm->cont = vm->pc + OpLength(op) + RawInt(ChunkConst(chunk, vm->pc+1));
      vm->pc += OpLength(op);
      break;
    case OpReturn:
      vm->pc = vm->cont;
      break;
    case OpSaveEnv:
      CallStackPush(vm, vm->env);
      vm->pc += OpLength(op);
      break;
    case OpRestEnv:
      vm->env = CallStackPop(vm);
      vm->pc += OpLength(op);
      break;
    case OpSaveCont:
      CallStackPush(vm, IntVal(vm->cont));
      vm->pc += OpLength(op);
      break;
    case OpRestCont:
      vm->cont = RawInt(CallStackPop(vm));
      vm->pc += OpLength(op);
      break;
    case OpApply: {
      Val func = StackPop(vm);
      if (IsPrimitive(func, mem)) {
        StackPush(vm, DoPrimitive(func, vm));
        vm->pc += OpLength(op);
      } else if (IsFunc(func, mem)) {
        vm->env = FuncEnv(func, mem);
        vm->pc = RawInt(FuncEntry(func, mem));
      } else {
        vm->pc += OpLength(op);
      }
      break;
    }
    case OpModule: {
      Val mod = StackPop(vm);
      TupleSet(vm->modules, RawInt(ChunkConst(chunk, vm->pc+1)), mod, mem);
      vm->pc += OpLength(op);
      break;
    }
    case OpLoad: {
      Val mod = TupleGet(vm->modules, RawInt(ChunkConst(chunk, vm->pc+1)), mem);
      StackPush(vm, mod);
      vm->pc += OpLength(op);
      break;
    }
    }
  }

  if (vm->error) {
    Print(VMErrorMessage(vm->error));
    Print("\n");
  } else if (vm->verbose) {
    TraceInstruction(vm, chunk);
    PrintMem(mem);
  }
}
