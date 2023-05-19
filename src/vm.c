#include "vm.h"
#include "chunk.h"
#include "env.h"
#include "primitives.h"
#include "ops.h"
#include "print.h"

static Val ArithmeticOp(OpCode op, VM *vm);
static void TraceInstruction(VM *vm, Chunk *chunk);

void InitVM(VM *vm, Mem *mem)
{
  vm->mem = mem;
  vm->pc = 0;
  vm->chunk = NULL;
  vm->halted = true;
  vm->stats.stack_ops = 0;

  vm->regs[RegVal] = nil;
  vm->regs[RegEnv] = InitialEnv(vm->mem);
  vm->regs[RegFun] = nil;
  vm->regs[RegArgs] = nil;
  vm->regs[RegCont] = nil;
  vm->regs[RegArg1] = nil;
  vm->regs[RegArg2] = nil;
  vm->regs[RegStack] = nil;

#if DEBUG_VM
  InitOps(mem);
#endif
}

void RunChunk(VM *vm, Chunk *chunk)
{
  Mem *mem = vm->mem;
  vm->pc = 0;
  vm->chunk = chunk;
  vm->halted = false;

  while (!vm->halted && vm->pc < VecCount(chunk->code)) {
    OpCode op = chunk->code[vm->pc];

    TraceInstruction(vm, chunk);

    switch (op) {
    case OpNoop:
      vm->pc++;
      break;
    case OpHalt:
      vm->halted = true;
      vm->pc++;
      return;
    case OpConst:
      vm->regs[ChunkRef(chunk, vm->pc+2)] = ChunkConst(chunk, vm->pc+1);
      vm->pc += OpLength(op);
      break;
    case OpMove:
      vm->regs[ChunkRef(chunk, vm->pc+2)] =
        vm->regs[ChunkRef(chunk, vm->pc+1)];
      vm->pc += OpLength(op);
      break;
    case OpBranch:
      if (IsTrue(vm->regs[ChunkRef(chunk, vm->pc+2)])) {
        vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      } else {
        vm->pc += OpLength(op);
      }
      break;
    case OpBranchF:
      if (!IsTrue(vm->regs[ChunkRef(chunk, vm->pc+2)])) {
        vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      } else {
        vm->pc += OpLength(op);
      }
      break;
    case OpJump:
      vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      break;
    case OpGoto:
      vm->pc = RawInt(vm->regs[ChunkRef(chunk, vm->pc+1)]);
      break;
    case OpPair:
      vm->regs[ChunkRef(chunk, vm->pc+2)] =
        MakePair(mem,
                ChunkConst(chunk, vm->pc+1),
                vm->regs[ChunkRef(chunk, vm->pc+2)]);
      vm->pc += OpLength(op);
      break;
    case OpHead:
      vm->regs[ChunkRef(chunk, vm->pc+1)] =
        Head(mem, vm->regs[ChunkRef(chunk, vm->pc+1)]);
      vm->pc += OpLength(op);
      break;
    case OpTail:
      vm->regs[ChunkRef(chunk, vm->pc+1)] =
        Tail(mem, vm->regs[ChunkRef(chunk, vm->pc+1)]);
      vm->pc += OpLength(op);
      break;
    case OpPush:
      if (ChunkRef(chunk, vm->pc+2) == RegStack) vm->stats.stack_ops++;
      vm->regs[ChunkRef(chunk, vm->pc+2)] =
        MakePair(mem,
                vm->regs[ChunkRef(chunk, vm->pc+1)],
                vm->regs[ChunkRef(chunk, vm->pc+2)]);
      vm->pc += OpLength(op);
      break;
    case OpPop:
      vm->regs[ChunkRef(chunk, vm->pc+2)] = Head(mem, vm->regs[ChunkRef(chunk, vm->pc+1)]);
      vm->regs[ChunkRef(chunk, vm->pc+1)] = Tail(mem, vm->regs[ChunkRef(chunk, vm->pc+1)]);
      vm->pc += OpLength(op);
      break;
    case OpLookup:
      vm->regs[ChunkRef(chunk, vm->pc+3)] =
        LookupByPosition(ChunkConst(chunk, vm->pc+1), ChunkConst(chunk, vm->pc+2), vm->regs[RegEnv], mem);
      if (Eq(vm->regs[ChunkRef(chunk, vm->pc+3)], SymbolFor("__undefined__"))) {
        RuntimeError("Undefined variable", MakePair(mem, ChunkConst(chunk, vm->pc+1), ChunkConst(chunk, vm->pc+2)), vm);
      }
      vm->pc += OpLength(op);
      break;
    case OpDefine:
      Define(ChunkConst(chunk, vm->pc+1), vm->regs[ChunkRef(chunk, vm->pc+2)], vm->regs[RegEnv], mem);
      vm->pc += OpLength(op);
      break;
    case OpPrim:
      vm->regs[ChunkRef(chunk, vm->pc+1)] = DoPrimitive(vm->regs[RegFun], vm->regs[RegArgs], vm);
      vm->pc += OpLength(op);
      break;
    case OpNot:
      vm->regs[ChunkRef(chunk, vm->pc+1)] = BoolVal(!IsTrue(vm->regs[ChunkRef(chunk, vm->pc+1)]));
      vm->pc += OpLength(op);
      break;
    case OpStr:
      vm->regs[ChunkRef(chunk, vm->pc+2)] =
        MakeBinaryFrom(mem, SymbolName(mem, ChunkConst(chunk, vm->pc+1)), SymbolLength(mem, ChunkConst(chunk, vm->pc+1)));
      vm->pc += OpLength(op);
      break;
    case OpEqual:
      vm->regs[ChunkRef(vm->chunk, vm->pc+1)] =
        BoolVal(Eq(vm->regs[RegArg1], vm->regs[RegArg2]));
      vm->pc += OpLength(op);
      break;
    case OpGt:
    case OpLt:
    case OpAdd:
    case OpSub:
    case OpMul:
    case OpDiv:
      vm->regs[ChunkRef(vm->chunk, vm->pc+1)] = ArithmeticOp(op, vm);
      vm->pc += OpLength(op);
      break;
    default:
      RuntimeError("Unimplemented op code", OpSymbol(op), vm);
      break;
    }
  }

  if (!vm->halted) {
    vm->halted = true;
    TraceInstruction(vm, chunk);
  }

#ifdef DEBUG_VM
  Print("Stack operations: ");
  PrintInt(vm->stats.stack_ops);
  Print("\n");
#endif
}

Val RuntimeError(char *message, Val exp, VM *vm)
{
  vm->halted = true;
  PrintEscape(IOFGRed);
  Print("(Runtime Error) ");
  Print(message);
  Print(": ");
  PrintVal(vm->mem, exp);
  PrintEscape(IOFGReset);
  Print("\n");
  return SymbolFor("error");
}

u32 PrintReg(i32 reg)
{
  switch (reg) {
  case RegVal:    return Print("[val]");
  case RegEnv:    return Print("[env]");
  case RegFun:    return Print("[fun]");
  case RegArgs:   return Print("[arg]");
  case RegCont:   return Print("[con]");
  case RegArg1:   return Print("[ar1]");
  case RegArg2:   return Print("[ar2]");
  case RegStack:  return Print("[stk]");
  default:        return Print("[???]");
  }
}

static void TraceInstruction(VM *vm, Chunk *chunk)
{
#if DEBUG_VM
  PrintIntN(vm->pc, 4, ' ');
  Print("│ ");
  i32 printed = PrintInstruction(chunk, vm->pc, vm->mem);

  for (i32 i = 0; i < 40 - printed; i++) Print(" ");
  Print("│ ");

  printed = 0;
  for (u32 i = 0; i < NUM_REGS; i++) {
    if (IsNil(vm->regs[i])) {
      printed += Print("nil");
    } else if (IsPair(vm->regs[i])) {
      printed += Print("p");
      printed += PrintInt(RawVal(vm->regs[i]));
    } else if (IsObj(vm->regs[i])) {
      printed += Print("v");
      printed += PrintInt(RawVal(vm->regs[i]));
    } else {
      printed += PrintVal(vm->mem, vm->regs[i]);
    }
    printed += Print(" ");
  }

  for (i32 i = 0; i < 40 - printed; i++) Print(" ");
  Print("│ ");

  Val stack = vm->regs[RegStack];
  for (i32 i = 0; i < 8 && !IsNil(stack); i++) {
    Val val = Head(vm->mem, stack);
    if (IsNil(val)) {
      printed += Print("nil ");
    } else if (IsPair(val)) {
      printed += Print("[");
      printed += PrintInt(ListLength(vm->mem, val));
      printed += Print("] ");
    } else {
      printed += PrintVal(vm->mem, val);
      printed += Print(" ");
    }

    stack = Tail(vm->mem, stack);
  }

  Print("\n");
#endif
}

static Val ArithmeticOp(OpCode op, VM *vm)
{
  Val a_val = vm->regs[RegArg1];
  Val b_val = vm->regs[RegArg2];

  if (!IsNumeric(a_val)) {
    return RuntimeError("Bad arithmetic argument", a_val, vm);
  }
  if (!IsNumeric(b_val)) {
    return RuntimeError("Bad arithmetic argument", b_val, vm);
  }

  if (IsInt(a_val) && IsInt(b_val) && op != OpDiv) {
    i32 a = RawInt(a_val);
    i32 b = RawInt(b_val);
    Val result;
    switch (op) {
    case OpAdd: result = IntVal(a + b); break;
    case OpSub: result = IntVal(a - b); break;
    case OpMul: result = IntVal(a * b); break;
    case OpGt:  result = BoolVal(a > b); break;
    case OpLt:  result = BoolVal(a < b); break;
    default:    result = nil; break;
    }
    return result;
  }

  float a = (float)RawNum(a_val);
  float b = (float)RawNum(b_val);
  Val result;
  switch (op) {
  case OpAdd: result = NumVal(a + b); break;
  case OpSub: result = NumVal(a - b); break;
  case OpMul: result = NumVal(a * b); break;
  case OpDiv: result = NumVal(a / b); break;
  case OpGt: result = BoolVal(a > b); break;
  case OpLt: result = BoolVal(a < b); break;
  default:    result = nil; break;
  }
  return result;
}
