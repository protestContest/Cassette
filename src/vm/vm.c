#include "vm.h"
#include "ops.h"
#include "env.h"
#include "function.h"
#include "rec.h"
#include "univ/math.h"
#include "univ/system.h"
#include "univ/memory.h"
#include "univ/string.h"
#include <time.h>

#define StackPeek(vm, n)  (vm)->stack[(vm)->stack_count - 1 - (n)]

void InitVM(VM *vm, CassetteOpts *opts)
{
  InitMem(&vm->mem, 0);
  vm->pc = 0;
  vm->cont = 0;
  vm->env = InitialEnv(&vm->mem);
  vm->stack_count = 0;
  vm->call_stack_count = 0;
  vm->mod_count = 0;
  vm->mod_map = EmptyHashMap;
  vm->error = false;
  vm->opts = opts;

  Assert(Eq(Ok, MakeSymbol("ok", &vm->mem)));
  Assert(Eq(Error, MakeSymbol("error", &vm->mem)));
  Assert(Eq(Function, MakeSymbol("*function*", &vm->mem)));
  Assert(Eq(Undefined, MakeSymbol("*undefined*", &vm->mem)));
  Assert(Eq(Moved, MakeSymbol("*moved*", &vm->mem)));
  SeedPrimitives();
  Seed(opts->seed);
}

void DestroyVM(VM *vm)
{
  // FreeVec(vm->stack);
}

char *VMErrorMessage(VMError err)
{
  switch (err) {
  case NoError:         return "NoError";
  case StackError:      return "StackError";
  case TypeError:       return "TypeError";
  case ArithmeticError: return "ArithmeticError";
  case EnvError:        return "EnvError";
  case KeyError:        return "KeyError";
  case ArityError:      return "ArityError";
  case RuntimeError:    return "RuntimeError";
  }
}

void StackPush(VM *vm, Val value)
{
  vm->stack[vm->stack_count++] = value;
}

Val StackPop(VM *vm)
{
  if (vm->stack_count == 0) {
    vm->error = StackError;
    return nil;
  }

  return vm->stack[--vm->stack_count];
}

static void CallStackPush(VM *vm, Val value)
{
  vm->call_stack[vm->call_stack_count++] = value;
}

static Val CallStackPop(VM *vm)
{
  if (vm->call_stack_count == 0) {
    vm->error = StackError;
    return nil;
  }

  return vm->call_stack[--vm->call_stack_count];
}

static u32 RunGC(VM *vm)
{
  Heap new_mem = BeginGC(&vm->mem);
  vm->env = CopyValue(vm->env, &vm->mem, &new_mem);
  for (u32 i = 0; i < vm->stack_count; i++) {
    vm->stack[i] = CopyValue(vm->stack[i], &vm->mem, &new_mem);
  }
  for (u32 i = 0; i < vm->call_stack_count; i++) {
    vm->call_stack[i] = CopyValue(vm->call_stack[i], &vm->mem, &new_mem);
  }
  for (u32 i = 0; i < vm->mod_count; i++) {
    vm->modules[i] = CopyValue(vm->modules[i], &vm->mem, &new_mem);
  }

  CollectGarbage(&vm->mem, &new_mem);
  Free(vm->mem.values);
  vm->mem = new_mem;

  return MemSize(&vm->mem);
}

static void ApplyValue(Val value, VM *vm)
{
  Heap *mem = &vm->mem;
  if (vm->stack_count == 0) {
    StackPush(vm, value);
    return;
  }

  Val args = StackPop(vm);

  if (IsPair(value) && TupleLength(args, mem) == 1) {
    Val arg = TupleGet(args, 0, mem);
    if (IsInt(arg)) {
      StackPush(vm, ListAt(value, RawInt(arg), mem));
    } else {
      vm->error = TypeError;
      StackPush(vm, arg);
    }
  } else if (IsTuple(value, mem) && TupleLength(args, mem) == 1) {
    Val arg = TupleGet(args, 0, mem);
    if (IsInt(arg)) {
      StackPush(vm, TupleGet(value, RawInt(arg), mem));
    } else {
      vm->error = TypeError;
      StackPush(vm, arg);
    }
  } else if (IsBinary(value, mem) && TupleLength(args, mem) == 1) {
    Val arg = TupleGet(args, 0, mem);
    if (IsInt(arg)) {
      StackPush(vm, IntVal(BinaryGetByte(value, RawInt(arg), mem)));
    } else {
      vm->error = TypeError;
      StackPush(vm, arg);
    }
  } else if (IsMap(value, mem) && TupleLength(args, mem) == 1) {
    Val arg = TupleGet(args, 0, mem);
    StackPush(vm, MapGet(value, arg, mem));
  } else {
    StackPush(vm, value);
  }
}

void RunChunk(VM *vm, Chunk *chunk)
{
  Heap *mem = &vm->mem;
  u32 next_gc = 2048;

  vm->pc = 0;
  vm->cont = ChunkSize(chunk);
  vm->stack_count = 0;
  vm->call_stack_count = 0;
  vm->mod_count = 0;
  vm->error = false;

  CopyStrings(&chunk->constants, mem);

  while (vm->error == NoError && vm->pc < ChunkSize(chunk)) {
    if (MemSize(mem) > next_gc) {
      RunGC(vm);
      next_gc = 2*MemSize(mem);
    }

    OpCode op = ChunkRef(chunk, vm->pc);

    switch (op) {
    case OpHalt:
      vm->pc = ChunkSize(chunk);
      break;
    case OpPop:
      StackPop(vm);
      if (!vm->error) vm->pc += OpLength(op);
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
      } else if (IsBinary(obj, mem)) {
        if (IsInt(key) && RawInt(key) >= 0
          && RawInt(key) < BinaryLength(obj, mem)) {
          char *data = BinaryData(obj, mem);
          StackPush(vm, IntVal(data[RawInt(key)]));
        } else {
          vm->error = KeyError;
        }
      } else {
        vm->error = TypeError;
      }
      if (!vm->error) vm->pc += OpLength(op);
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
      if (!vm->error) vm->pc += OpLength(op);
      break;
    }
    case OpNot:
      StackPush(vm, BoolVal(IsTrue(StackPop(vm))));
      if (!vm->error) vm->pc += OpLength(op);
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
      if (!vm->error) vm->pc += OpLength(op);
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
      if (!vm->error) vm->pc += OpLength(op);
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
      if (!vm->error) vm->pc += OpLength(op);
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
      if (!vm->error) vm->pc += OpLength(op);
      break;
    }
    case OpAdd: {
      Val a = StackPop(vm);
      if (!IsNum(a)) {
        vm->error = TypeError;
        break;
      }
      Val b = StackPop(vm);
      if (!IsNum(b)) {
        vm->error = TypeError;
        break;
      }
      if (IsInt(a) && IsInt(b)) {
        StackPush(vm, IntVal(RawInt(a) + RawInt(b)));
      } else {
        StackPush(vm, FloatVal(RawNum(a) + RawNum(b)));
      }
      if (!vm->error) vm->pc += OpLength(op);
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
      if (!vm->error) vm->pc += OpLength(op);
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
      if (!vm->error) vm->pc += OpLength(op);
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
      if (!vm->error) vm->pc += OpLength(op);
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
      if (!vm->error) vm->pc += OpLength(op);
      break;
    }
    case OpEq:
      StackPush(vm, BoolVal(Eq(StackPop(vm), StackPop(vm))));
      if (!vm->error) vm->pc += OpLength(op);
      break;
    case OpPair: {
      Val head = StackPop(vm);
      Val tail = StackPop(vm);
      StackPush(vm, Pair(head, tail, mem));
      if (!vm->error) vm->pc += OpLength(op);
      break;
    }
    case OpTuple: {
      u32 len = RawInt(ChunkConst(chunk, vm->pc+1));
      Val tuple = MakeTuple(len, mem);
      StackPush(vm, tuple);
      if (!vm->error) vm->pc += OpLength(op);
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
      if (!vm->error) vm->pc += OpLength(op);
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
      if (!vm->error) vm->pc += OpLength(op);
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
      if (!vm->error) vm->pc += OpLength(op);
      break;
    }
    case OpLambda: {
      u32 size = RawInt(ChunkConst(chunk, vm->pc+1));
      StackPush(vm, MakeFunc(IntVal(vm->pc+2), vm->env, mem));
      if (!vm->error) vm->pc += OpLength(op) + size;
      break;
    }
    case OpExtend: {
      Val frame = StackPop(vm);
      vm->env = ExtendEnv(vm->env, frame, mem);
      if (!vm->error) vm->pc += OpLength(op);
      break;
    }
    case OpDefine:
      Define(RawInt(ChunkConst(chunk, vm->pc+1)), StackPop(vm), vm->env, mem);
      if (!vm->error) vm->pc += OpLength(op);
      break;
    case OpLookup: {
      u32 frame = RawInt(ChunkConst(chunk, vm->pc+1));
      u32 pos = RawInt(ChunkConst(chunk, vm->pc+2));
      Val value = Lookup(vm->env, frame, pos, mem);
      if (Eq(value, Undefined)) {
        vm->error = EnvError;
      } else {
        StackPush(vm, value);
      }
      if (!vm->error) vm->pc += OpLength(op);
      break;
    }
    case OpExport:
      StackPush(vm, Head(vm->env, mem));
      if (!vm->error) vm->pc += OpLength(op);
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
      if (!vm->error) vm->pc += OpLength(op);
      break;
    case OpSaveCont:
      CallStackPush(vm, IntVal(vm->cont));
      vm->pc += OpLength(op);
      break;
    case OpRestCont:
      vm->cont = RawInt(CallStackPop(vm));
      if (!vm->error) vm->pc += OpLength(op);
      break;
    case OpApply: {
      Val func = StackPop(vm);
      if (IsPrimitive(func, mem)) {
        StackPush(vm, DoPrimitive(func, vm));
        if (!vm->error) vm->pc = vm->cont;
      } else if (IsFunc(func, mem)) {
        vm->env = FuncEnv(func, mem);
        vm->pc = RawInt(FuncEntry(func, mem));
      } else {
        ApplyValue(func, vm);
        if (!vm->error) vm->pc = vm->cont;
      }
      break;
    }
    case OpModule: {
      Val mod = StackPop(vm);
      Val mod_name = ChunkConst(chunk, vm->pc+1);
      if (HashMapContains(&vm->mod_map, mod_name.as_i)) {
        u32 index = HashMapGet(&vm->mod_map, mod_name.as_i);
        vm->modules[index] = mod;
      } else {
        HashMapSet(&vm->mod_map, mod_name.as_i, vm->mod_count);
        vm->modules[vm->mod_count++] = mod;
      }

      if (!vm->error) vm->pc += OpLength(op);
      break;
    }
    case OpLoad: {
      Val mod_name = ChunkConst(chunk, vm->pc+1);
      if (HashMapContains(&vm->mod_map, mod_name.as_i)) {
        u32 index = HashMapGet(&vm->mod_map, mod_name.as_i);
        StackPush(vm, vm->modules[index]);
      } else {
        vm->error = EnvError;
      }

      if (!vm->error) vm->pc += OpLength(op);
      break;
    }
    }
  }
}
