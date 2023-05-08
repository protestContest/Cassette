#include "vm.h"
#include "chunk.h"
#include "env.h"
#include "port.h"
#include "primitives.h"
#include "ops.h"

u32 PrintReg(i32 reg)
{
  switch (reg) {
  case RegVal:  return Print("[val]");
  case RegEnv:  return Print("[env]");
  case RegCon:  return Print("[con]");
  case RegFun:  return Print("[fun]");
  case RegArg:  return Print("[arg]");
  default:      return Print("[???]");
  }
}

void InitVM(VM *vm, Mem *mem)
{
  vm->mem = mem;
  vm->stack = NULL;
  vm->pc = 0;
  vm->chunk = NULL;
  vm->modules = nil;
  vm->trace = false;
  vm->stats.stack_ops = 0;
  vm->stats.reductions = 0;

  vm->regs[RegVal] = nil;
  vm->regs[RegEnv] = InitialEnv(vm->mem);
  vm->regs[RegFun] = nil;
  vm->regs[RegArg] = nil;
  vm->regs[RegCon] = nil;
}

void TraceVM(VM *vm)
{
  vm->trace = true;
}

static void TraceInstruction(VM *vm, Chunk *chunk)
{
  PrintIntN(vm->pc, 4, ' ');
  Print("│ ");
  i32 printed = PrintInstruction(chunk, vm->pc, vm->mem);

  for (i32 i = 0; i < 40 - printed; i++) Print(" ");
  Print("│ ");

  printed = 0;
  for (u32 i = 0; i < NUM_REGS; i++) {
    printed += PrintVal(vm->mem, vm->regs[i]);
    printed += Print(" ");
  }

  for (i32 i = 0; i < 40 - printed; i++) Print(" ");
  Print("│ ");

  for (u32 i = 0; i < 8; i++) {
    if (i >= VecCount(vm->stack)) break;
    PrintVal(vm->mem, vm->stack[VecCount(vm->stack) - i - 1]);
    Print(" ");
  }

  Print("\n");
}

void RunChunk(VM *vm, Chunk *chunk)
{
  vm->pc = 0;
  vm->chunk = chunk;

  while (vm->pc < VecCount(chunk->data)) {
    OpCode op = chunk->data[vm->pc];

    if (vm->trace) {
      TraceInstruction(vm, chunk);
    }

    switch (op) {
    case OpNoop:
      vm->pc++;
      break;
    case OpConst:
      vm->regs[ChunkRef(chunk, vm->pc+2)] = ChunkConst(chunk, vm->pc+1);
      vm->pc += OpLength(op);
      break;
    case OpLookup:
      vm->regs[ChunkRef(chunk, vm->pc+2)] =
        Lookup(ChunkConst(chunk, vm->pc+1), vm->regs[RegEnv], vm);
      vm->pc += OpLength(op);
      break;
    case OpDefine:
      Define(ChunkConst(chunk, vm->pc+1), vm->regs[RegVal], vm->regs[RegEnv], vm->mem);
      vm->pc += OpLength(op);
      break;
    case OpBranch:
      if (!IsTrue(vm->regs[RegVal])) {
        vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      } else {
        vm->pc += OpLength(op);
      }
      break;
    case OpNot:
      vm->regs[RegVal] = BoolVal(!IsTrue(vm->regs[RegVal]));
      vm->pc += OpLength(op);
      break;
    case OpLambda:
      vm->regs[ChunkRef(chunk, vm->pc+2)] =
        MakeProcedure(ChunkConst(chunk, vm->pc+1), vm->regs[RegEnv], vm->mem);
      vm->pc += OpLength(op);
      break;
    case OpDefArg:
      Define(ChunkConst(chunk, vm->pc+1), Head(vm->mem, vm->regs[RegArg]), vm->regs[RegEnv], vm->mem);
      vm->regs[RegArg] = Tail(vm->mem, vm->regs[RegArg]);
      vm->pc += OpLength(op);
      break;
    case OpExtEnv:
      vm->regs[RegEnv] = ExtendEnv(vm->regs[RegEnv], vm->mem);
      vm->pc++;
      break;
    case OpPushArg:
      vm->regs[RegArg] = MakePair(vm->mem, vm->regs[RegVal], vm->regs[RegArg]);
      vm->pc++;
      break;
    case OpBrPrim:
      if (IsPrimitive(vm->regs[RegFun], vm->mem)) {
        vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      } else {
        vm->pc += OpLength(op);
      }
      break;
    case OpPrim:
      vm->regs[ChunkRef(chunk, vm->pc+1)] =
        DoPrimitive(vm->regs[RegFun], vm->regs[RegArg], vm);
      vm->pc += OpLength(op);
      break;
    case OpApply:
      vm->stats.reductions++;
      vm->pc = RawInt(ProcBody(vm->regs[RegFun], vm->mem));
      break;
    case OpMove:
      vm->regs[ChunkRef(chunk, vm->pc+2)] = vm->regs[ChunkRef(chunk, vm->pc+1)];
      vm->pc += OpLength(op);
      break;
    case OpPush:
      vm->stats.stack_ops++;
      VecPush(vm->stack, vm->regs[ChunkRef(chunk, vm->pc+1)]);
      vm->pc += OpLength(op);
      break;
    case OpPop:
      vm->stats.stack_ops++;
      if (VecCount(vm->stack) == 0) {
        RuntimeError("Stack underflow", nil, vm);
      } else {
        vm->regs[ChunkRef(chunk, vm->pc+1)] = VecPop(vm->stack);
      }
      vm->pc += OpLength(op);
      break;
    case OpJump:
      vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      break;
    case OpReturn:
      vm->pc = RawInt(vm->regs[RegCon]);
      break;
    case OpHalt:
      Halt(vm);
      break;
    case OpTrace:
      vm->trace = true;
      break;
    default:
      RuntimeError("Invalid op code", IntVal(op), vm);
      break;
    }
  }

#ifdef DEBUG_VM
  Print("Stack operations: ");
  PrintInt(vm->stats.stack_ops);
  Print("\nReductions: ");
  PrintInt(vm->stats.reductions);
  Print("\n");
#endif
}

Val RuntimeError(char *message, Val exp, VM *vm)
{
  PrintEscape(IOFGRed);
  Print("(Runtime Error) ");
#ifdef DEBUG_VM
  PrintInt(vm->pc);
  Print(" ");
#endif
  Print(message);
  Print(": ");
  PrintVal(vm->mem, exp);
  PrintEscape(IOFGReset);
  Print("\n");
  Halt(vm);
  return SymbolFor("error");
}
