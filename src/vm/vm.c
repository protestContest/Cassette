#include "vm.h"
#include "ops.h"
#include "env.h"
#include "function.h"
#include "lib.h"
#include "debug.h"
#include "rec.h"
#include <time.h>

static u32 **op_data = NULL;
static HashMap op_stats = EmptyHashMap;

void InitVM(VM *vm, Args *args, Heap *mem)
{
  vm->pc = 0;
  vm->cont = 0;
  vm->stack = NewVec(Val, 256);
  vm->call_stack = NewVec(Val, 256);
  vm->env = InitialEnv(mem);
  vm->modules = NULL;
  vm->mod_map = EmptyHashMap;
  vm->mem = mem;
  vm->error = false;
  vm->args = args;

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
  case NoError:         return "NoError";
  case StackError:      return "StackError";
  case TypeError:       return "TypeError";
  case ArithmeticError: return "ArithmeticError";
  case EnvError:        return "EnvError";
  case KeyError:        return "KeyError";
  case ArgError:        return "ArgError";
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
  Val *roots[3];
  roots[0] = vm->stack;
  roots[1] = vm->call_stack;
  roots[2] = vm->modules;
  CollectGarbage(roots, 3, vm->mem);
  vm->env = StackPop(vm);
  return MemSize(vm->mem);
}

static void PrintCurrentToken(VM *vm, Chunk *chunk)
{
  char *file = SourceFile(vm->pc, chunk);
  if (file) {
    u32 pos = SourcePos(vm->pc, chunk);
    char *source = ReadFile(file);
    Lexer lex;
    InitLexer(&lex, source, pos);

    PrintToken(lex.token);
    // PrintSourceContext(source, pos);
  }
}

static void RecordOp(OpCode op, u32 time)
{
  if (HashMapContains(&op_stats, op)) {
    u32 index = HashMapGet(&op_stats, op);
    VecPush(op_data[index], time);
  } else {
    u32 index = VecCount(op_data);
    HashMapSet(&op_stats, op, index);
    VecPush(op_data, NULL);
    VecPush(op_data[index], time);
  }
}

void PrintOpStats(void)
{
  for (u32 i = 0; i < op_stats.count; i++) {
    u32 op = HashMapKey(&op_stats, i);
    u32 *samples = op_data[HashMapGet(&op_stats, op)];
    if (VecCount(samples) < 10) continue;

    float avg = 0;
    for (u32 j = 0; j < VecCount(samples); j++) {
      avg += samples[j];
    }
    avg /= VecCount(samples);
    float var = 0;
    for (u32 j = 0; j < VecCount(samples); j++) {
      u32 delta = samples[j] - avg;
      var += delta * delta;
    }
    var /= VecCount(samples) - 1;
    float std = Sqrt(var);
    // float z = 1.960;
    // float confidence = z * std / Sqrt(avg);

    u32 written = Print(OpName(op));
    for (u32 i = 0; i < 16 - written; i++) Print(" ");
    PrintFloat(avg, 3);
    Print("μs σ");
    PrintFloat(std, 3);
    Print(" (");
    PrintInt(VecCount(samples));
    Print(")\n");
  }
}

#define GCFreq  1024
void RunChunk(VM *vm, Chunk *chunk)
{
  Heap *mem = vm->mem;
  u32 gc = GCFreq;

  vm->pc = 0;
  vm->cont = ChunkSize(chunk);
  vm->stack = NULL;
  vm->error = false;

  CopyStrings(&chunk->constants, mem);

  if (vm->args->verbose) {
    TraceHeader();
  }

  while (vm->error == NoError && vm->pc < ChunkSize(chunk)) {
    if (vm->args->verbose) {
      TraceInstruction(vm, chunk);
    }

    if (--gc == 0) {
      gc = GCFreq;
      u32 start = MemSize(mem);
      if (vm->args->verbose) {
        Print("GARBAGE DAY!!");
      }
      RunGC(vm);
      u32 cleaned = start - MemSize(mem);
      if (vm->args->verbose) {
        Print(" Collected ");
        PrintInt(cleaned);
        Print("\n");
      }
    }

    OpCode op = ChunkRef(chunk, vm->pc);
    u32 start = clock();
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
      if (Eq(value, SymbolFor("*undefined*"))) {
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
        if (!vm->error) vm->pc += OpLength(op);
      }
      u32 dt = clock() - start;
      if (IsFunc(func, mem)) RecordOp(op, dt);
      break;
    }
    case OpModule: {
      Val mod = StackPop(vm);
      Val mod_name = ChunkConst(chunk, vm->pc+1);
      if (HashMapContains(&vm->mod_map, mod_name.as_i)) {
        u32 index = HashMapGet(&vm->mod_map, mod_name.as_i);
        vm->modules[index] = mod;
      } else {
        u32 index = VecCount(vm->modules);
        VecPush(vm->modules, mod);
        HashMapSet(&vm->mod_map, mod_name.as_i, index);
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

    u32 dt = clock() - start;
    if (op != OpApply) RecordOp(op, dt);
  }

  if (vm->error) {
    PrintEscape(IOFGRed);
    Print(VMErrorMessage(vm->error));

    if (vm->error == TypeError && VecCount(vm->stack) > 0) {
      Print(" (");
      Inspect(StackPeek(vm, 0), mem);
      Print(")");
    }

    char *file = SourceFile(vm->pc, chunk);
    if (file) {
      Print(" in ");
      Print(file);
      Print(":\n");
      char *source = ReadFile(file);

      u32 pos = SourcePos(vm->pc, chunk);

      PrintSourceContext(source, pos);
    } else {
      u32 start = (vm->pc > 5) ? vm->pc - 5 : 0;
      DisassemblePart(chunk, start, 10);
      Print("\n");
    }
    PrintEscape(IOFGReset);
  } else if (vm->args->verbose) {
    TraceInstruction(vm, chunk);
    if (vm->args->verbose > 1) {
      PrintMem(mem);
    }
  }
}
