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
  case RegCont: return Print("[con]");
  case RegFun:  return Print("[fun]");
  case RegArgs: return Print("[arg]");
  case RegArg1: return Print("[ar1]");
  case RegArg2: return Print("[ar2]");
  default:      return Print("[???]");
  }
}

void InitVM(VM *vm, Mem *mem)
{
  vm->mem = mem;
  vm->pc = 0;
  vm->chunk = NULL;
  vm->condition = false;
  vm->stats.stack_ops = 0;
  vm->stats.reductions = 0;

  vm->regs[RegVal] = nil;
  vm->regs[RegEnv] = InitialEnv(vm->mem);
  vm->regs[RegFun] = nil;
  vm->regs[RegArgs] = nil;
  vm->regs[RegCont] = nil;
  vm->regs[RegArg1] = nil;
  vm->regs[RegArg2] = nil;
  vm->regs[RegStack] = nil;
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

  Print("\n");
}

void RunChunk(VM *vm, Chunk *chunk)
{
  vm->pc = 0;
  vm->chunk = chunk;

  while (vm->pc < VecCount(chunk->data)) {
    OpCode op = chunk->data[vm->pc];

    TraceInstruction(vm, chunk);

    switch (op) {
    case OpNoop:
      vm->pc++;
      break;
    case OpHalt:
      vm->pc++;
      return;
    case OpConst:
      vm->regs[ChunkRef(chunk, vm->pc+2)] = ChunkConst(chunk, vm->pc+1);
      vm->pc += OpLength(op);
      break;
    case OpTest:
      vm->condition = IsTrue(vm->regs[vm->pc+1]);
      vm->pc += OpLength(op);
      break;
    case OpNot:
      vm->condition = !IsTrue(vm->regs[vm->pc+1]);
      vm->pc += OpLength(op);
      break;
    case OpBranch:
      if (vm->condition) {
        vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      } else {
        vm->pc += OpLength(op);
      }
      break;
    case OpJump:
      vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      break;
    case OpGoto:
      vm->pc += RawInt(vm->regs[ChunkRef(chunk, vm->pc+1)]);
      break;
    case OpPush:
      vm->regs[ChunkRef(chunk, vm->pc+2)] =
        MakePair(vm->mem,
                vm->regs[ChunkRef(chunk, vm->pc+1)],
                vm->regs[ChunkRef(chunk, vm->pc+2)]);
      vm->pc += OpLength(op);
      break;
    case OpPop:
      vm->regs[ChunkRef(chunk, vm->pc+1)] = Head(vm->mem, vm->regs[ChunkRef(chunk, vm->pc+2)]);
      vm->regs[ChunkRef(chunk, vm->pc+2)] = Tail(vm->mem, vm->regs[ChunkRef(chunk, vm->pc+2)]);
      vm->pc += OpLength(op);
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
  return SymbolFor("error");
}
