#include "vm.h"
#include "ops.h"
#include "env.h"
#include "proc.h"
#include "primitives.h"

void InitVM(VM *vm)
{
  InitMem(&vm->mem);
  InitMap(&vm->modules);

  vm->pc = 0;
  vm->cont = nil;
  vm->val = NULL;
  vm->stack = NULL;
  vm->chunk = NULL;
  vm->env = InitialEnv(&vm->mem);
  vm->error = false;
  vm->trace = false;
  vm->gc_threshold = 1024;
}

void DestroyVM(VM *vm)
{
  DestroyMem(&vm->mem);
  DestroyMap(&vm->modules);
  FreeVec(vm->val);
  FreeVec(vm->stack);
  vm->chunk = NULL;
  vm->cont = nil;
  vm->error = false;
  vm->trace = false;
}

Val StackPop(VM *vm)
{
  return VecPop(vm->val);
}

Val StackPeek(VM *vm, u32 n)
{
  return vm->val[VecCount(vm->val) - 1 - n];
}

void StackPush(VM *vm, Val val)
{
  VecPush(vm->val, val);
}

static void MergeStrings(Mem *dst, Mem *src)
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
  PrintEscape(IOFGRed);
  Print("Runtime error: ");
  Print(message);
  PrintEscape(IOFGReset);
  Print("\n");
  vm->error = true;
  Halt(vm);
}

void TraceInstruction(VM *vm)
{
  PrintIntN(vm->pc, 4, ' ');
  Print("│ ");
  u32 written = PrintInstruction(vm->chunk, vm->pc);

  if (written < 20) {
    for (u32 i = 0; i < 20 - written; i++) Print(" ");
  }

  Print(" │ ");
  PrintIntN(RawInt(vm->cont), 4, ' ');
  Print(" ║ ");

  for (u32 i = 0; i < VecCount(vm->val) && i < 8; i++) {
    InspectVal(vm->val[i], &vm->mem);
    Print(" │ ");
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
  case OpExp: {
    i32 result = 1;
    for (i32 i = 0; i < b; i++) result *= a;
    return result;
  }
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

static void CreateOp(VM *vm, OpCode op)
{
  Mem *mem = &vm->mem;
  Chunk *chunk = vm->chunk;

  switch (op) {
  case OpStr:
    StackPush(vm, MakeBinary(SymbolName(StackPop(vm), mem), mem));
    break;
  case OpPair: {
    Val tail = StackPop(vm);
    Val head = StackPop(vm);
    Val pair = Pair(head, tail, mem);
    StackPush(vm, pair);
    break;
  }
  case OpList: {
    Val list = nil;
    for (u32 i = 0; i < RawInt(ChunkConst(chunk, vm->pc+1)); i++) {
      list = Pair(StackPop(vm), list, mem);
    }
    StackPush(vm, list);
    break;
  }
  case OpTuple: {
    u32 length = RawInt(ChunkConst(chunk, vm->pc+1));
    Val tuple = MakeTuple(length, mem);
    for (u32 i = 0; i < length; i++) {
      TupleSet(tuple, i, StackPeek(vm, length - i - 1), mem);
    }
    RewindVec(vm->val, length);
    StackPush(vm, tuple);
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
    break;
  }
  default: Abort();
  }
}

static void CompareOp(VM *vm, OpCode op)
{
  Val b = StackPop(vm);
  Val a = StackPop(vm);

  if (op == OpEq) {
    Val res = BoolVal(Eq(a, b));
    StackPush(vm, res);
    return;
  }

  if (!IsNumeric(a) || !IsNumeric(b)) {
    RuntimeError(vm, "Bad arithmetic argument");
    return;
  }

  if (op == OpLt) StackPush(vm, BoolVal(RawNum(a) < RawNum(b)));
  else if (op == OpGt) StackPush(vm, BoolVal(RawNum(a) > RawNum(b)));
}

static void AccessOp(VM *vm, Val obj, Val arg)
{
  if (IsPair(obj)) {
    if (!IsInt(arg)) {
      RuntimeError(vm, "Must access lists with integers");
    } else {
      StackPush(vm, ListAt(obj, RawInt(arg), &vm->mem));
    }
  } else if (IsTuple(obj, &vm->mem)) {
    if (!IsInt(arg)) {
      RuntimeError(vm, "Must access tuples with integers");
    } else {
      StackPush(vm, TupleGet(obj, RawInt(arg), &vm->mem));
    }
  } else if (IsBinary(obj, &vm->mem)) {
    if (!IsInt(arg)) {
      RuntimeError(vm, "Must access binaries with integers");
    } else {
      char byte = BinaryData(obj, &vm->mem)[RawInt(arg)];
      StackPush(vm, IntVal(byte));
    }
  } else if (IsValMap(obj, &vm->mem)) {
    StackPush(vm, ValMapGet(obj, arg, &vm->mem));
  }
}

Val RunChunk(VM *vm, Chunk *chunk)
{
  vm->cont = nil;
  vm->val = NULL;
  vm->stack = NULL;
  vm->chunk = chunk;
  MergeStrings(&vm->mem, &chunk->constants);
  vm->error = false;
  Mem *mem = &vm->mem;

  while (vm->pc < VecCount(chunk->data)) {
    if (vm->trace) TraceInstruction(vm);

    if (MemSize(mem) > vm->gc_threshold) {
      if (vm->trace) Print("Collecting garbage...\n");
      TakeOutGarbage(vm);
      vm->gc_threshold = 2*MemSize(mem);
    }

    OpCode op = ChunkRef(chunk, vm->pc);

    switch (op) {
    case OpConst:
      StackPush(vm, ChunkConst(chunk, vm->pc+1));
      vm->pc += OpLength(op);
      break;
    case OpStr:
    case OpPair:
    case OpList:
    case OpTuple:
    case OpMap:
      CreateOp(vm, op);
      vm->pc += OpLength(op);
      break;
    case OpLen: {
      Val val = StackPop(vm);
      if (IsPair(val)) StackPush(vm, IntVal(ListLength(val, mem)));
      else if (IsTuple(val, mem)) StackPush(vm, IntVal(TupleLength(val, mem)));
      else if (IsBinary(val, mem)) StackPush(vm, IntVal(BinaryLength(val, mem)));
      else if (IsValMap(val, mem)) StackPush(vm, IntVal(ValMapCount(val, mem)));
      else {
        PrintVal(val, mem);
        Print("\n");
        RuntimeError(vm, "Argument has no length");
      }
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
    case OpSub:
    case OpMul:
    case OpDiv:
    case OpExp:
      ArithmeticOp(vm, op);
      vm->pc += OpLength(op);
      break;
    case OpNeg:
      if (!IsNumeric(StackPeek(vm, 0))) {
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
    case OpGt:
    case OpLt:
      CompareOp(vm, op);
      vm->pc += OpLength(op);
      break;
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
      StackPush(vm, MakeFunction(pos, vm->env, mem));
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
      u32 num_args = RawInt(ChunkConst(chunk, vm->pc+1));
      Val proc = StackPop(vm);
      if (IsPrimitive(proc, mem)) {
        StackPush(vm, ApplyPrimitive(proc, num_args - 1, vm));
        vm->pc = RawInt(vm->cont);
      } else if (IsFunction(proc, mem)) {
        vm->pc = ProcEntry(proc, mem);
        vm->env = ExtendEnv(ProcEnv(proc, mem), mem);
      } else if (num_args == 0) {
        StackPush(vm, proc);
        vm->pc += OpLength(op);
      } else {
        if (num_args == 2 && (IsPair(proc) || IsObj(proc))) {
          Val arg = StackPop(vm);
          AccessOp(vm, proc, arg);
        } else {
          RuntimeError(vm, "Not a function");
        }
        vm->pc += OpLength(op);
      }
      break;
    }
    case OpReturn:
      vm->pc = RawInt(vm->cont);
      break;
    case OpLookup: {
      Val value = Lookup(ChunkConst(chunk, vm->pc+1), vm->env, &vm->mem);
      if (Eq(value, SymbolFor("*undefined*"))) {
        RuntimeError(vm, "Undefined variable");
      } else {
        StackPush(vm, value);
        vm->pc += OpLength(op);
      }
      break;
    }
    case OpDefine: {
      Val id = ChunkConst(chunk, vm->pc+1);
      Val value = StackPop(vm);
      Define(id, value, vm->env, &vm->mem);
      StackPush(vm, id);
      vm->pc += OpLength(op);
      break;
    }
    case OpJump:
      vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      break;
    case OpBranch: {
      Val test = StackPeek(vm, 0);
      if (IsTrue(test)) {
        vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      } else {
        vm->pc += OpLength(op);
      }
      break;
    }
    case OpBranchF: {
      Val test = StackPeek(vm, 0);
      if (!IsTrue(test)) {
        vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      } else {
        vm->pc += OpLength(op);
      }
      break;
    }
    case OpPop:
      RewindVec(vm->val, 1);
      vm->pc += OpLength(op);
      break;
    case OpHalt:
      vm->pc = VecCount(chunk->data);
      break;
    case OpDefMod: {
      Val name = ChunkConst(chunk, vm->pc+1);
      Val mod = StackPeek(vm, 0);
      MapSet(&vm->modules, name.as_i, mod.as_i);
      vm->pc += OpLength(op);
      break;
    }
    case OpGetMod: {
      Val name = ChunkConst(chunk, vm->pc+1);
      if (MapContains(&vm->modules, name.as_i)) {
        Val mod = (Val){.as_i = MapGet(&vm->modules, name.as_i)};
        StackPush(vm, mod);
        vm->pc += OpLength(op);
      } else {
        RuntimeError(vm, "Module not found");
      }
      break;
    }
    case OpExport:
      StackPush(vm, ExportEnv(vm->env, mem));
      vm->pc += OpLength(op);
      break;
    case OpImport: {
      Val mod = StackPop(vm);
      ImportEnv(mod, vm->env, mem);
      StackPush(vm, MakeSymbol("ok", mem));
      vm->pc += OpLength(op);
      break;
    }
    }
  }

  if (!vm->error && VecCount(vm->val) > 0) {
    Val result = StackPop(vm);
    return result;
  } else {
    return nil;
  }
}

void TakeOutGarbage(VM *vm)
{
  u32 val_size = VecCount(vm->val);
  u32 stack_size = VecCount(vm->stack);
  u32 num_mods = MapCount(&vm->modules);
  u32 num_roots = num_mods + val_size + stack_size + 1;

  Val *roots = NewVec(Val, num_roots);
  RawVecCount(roots) = num_roots;

  roots[0] = vm->env;

  Copy(vm->val, roots+1, val_size*sizeof(Val));
  Copy(vm->stack, roots+1+val_size, stack_size*sizeof(Val));

  for (u32 i = 0; i < num_mods; i++) {
    Val mod = (Val){.as_i = GetMapValue(&vm->modules, i)};
    roots[i + 1 + val_size + stack_size] = mod;
  }

  CollectGarbage(roots, &vm->mem);

  vm->env = roots[0];
  Copy(roots+1, vm->val, val_size*sizeof(Val));
  Copy(roots+1+val_size, vm->stack, stack_size*sizeof(Val));

  for (u32 i = 0; i < num_mods; i++) {
    u32 key = GetMapKey(&vm->modules, i);
    MapSet(&vm->modules, key, roots[i+1+val_size+stack_size].as_i);
  }

  FreeVec(roots);
}

void PrintStack(VM *vm)
{
  Print("┌╴Stack╶─────────────\n");
  if (VecCount(vm->val) == 0) {
    Print("│ (empty)\n");
  }
  for (u32 i = 0; i < VecCount(vm->val); i++) {
    Print("│");
    InspectVal(vm->val[i], &vm->mem);
    Print("\n");
  }
  Print("└────────────────────\n");
}

void PrintCallStack(VM *vm)
{
  Print("┌╴Call Stack╶────────\n");
  if (VecCount(vm->stack) == 0) {
    Print("│ (empty)\n");
  }
  for (u32 i = 0; i < VecCount(vm->stack); i++) {
    Print("│");
    InspectVal(vm->stack[i], &vm->mem);
    Print("\n");
  }
  Print("└────────────────────\n");
}
