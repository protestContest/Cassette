#include "vm.h"
#include "chunk.h"
#include "env.h"
#include "port.h"
#include "primitives.h"

u32 PrintReg(i32 reg)
{
  switch (reg) {
  case RegVal:  return Print("[val]");
  case RegExp:  return Print("[exp]");
  case RegEnv:  return Print("[env]");
  case RegCont: return Print("[con]");
  case RegProc: return Print("[fun]");
  case RegArgs: return Print("[arg]");
  default:      return Print("[???]");
  }
}

void InitVM(VM *vm, Mem *mem)
{
  *vm = (VM){mem, NULL, 0, {nil, nil, nil, nil, nil, nil}, true};
  vm->regs[2] = InitialEnv(vm->mem);
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
  vm->halted = false;

  while (!vm->halted) {
    OpCode op = chunk->data[vm->pc];

#ifdef DEBUG_VM
    TraceInstruction(vm, chunk);
#endif

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
        Lookup(ChunkConst(chunk, vm->pc+1), vm->regs[2], vm);
      vm->pc += OpLength(op);
      break;
    case OpDefine:
      Define(ChunkConst(chunk, vm->pc+1), vm->regs[0], vm->regs[2], vm->mem);
      vm->regs[0] = SymbolFor("ok");
      vm->pc += OpLength(op);
      break;
    case OpBranch:
      if (!IsTrue(vm->regs[0])) {
        vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      } else {
        vm->pc += OpLength(op);
      }
      break;
    case OpNot:
      vm->regs[0] = BoolVal(!IsTrue(vm->regs[0]));
      vm->pc += OpLength(op);
      break;
    case OpLambda:
      vm->regs[ChunkRef(chunk, vm->pc+2)] =
        MakeProcedure(ChunkConst(chunk, vm->pc+1), vm->regs[2], vm->mem);
      vm->pc += OpLength(op);
      break;
    case OpDefArg:
      Define(ChunkConst(chunk, vm->pc+1), Head(vm->mem, vm->regs[5]), vm->regs[2], vm->mem);
      vm->regs[5] = Tail(vm->mem, vm->regs[5]);
      vm->pc += OpLength(op);
      break;
    case OpExtEnv:
      vm->regs[2] = ExtendEnv(vm->regs[2], vm->mem);
      vm->pc++;
      break;
    case OpPushArg:
      vm->regs[5] = MakePair(vm->mem, vm->regs[0], vm->regs[5]);
      vm->pc++;
      break;
    case OpBrPrim:
      if (IsPrimitive(vm->regs[4], vm->mem)) {
        vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      } else {
        vm->pc += OpLength(op);
      }
      break;
    case OpPrim:
      vm->regs[ChunkRef(chunk, vm->pc+1)] =
        DoPrimitive(vm->regs[4], vm->regs[5], vm);
      vm->pc += OpLength(op);
      break;
    case OpApply:
      vm->pc = RawInt(ProcBody(vm->regs[4], vm->mem));
      break;
    // case OpMove:    break;
    case OpPush:
      VecPush(vm->stack, vm->regs[ChunkRef(chunk, vm->pc+1)]);
      vm->pc += OpLength(op);
      break;
    case OpPop:
      if (VecCount(vm->stack) == 0) {
        RuntimeError("Stack underflow", nil, vm);
        return;
      } else {
        vm->regs[ChunkRef(chunk, vm->pc+1)] =
          VecPop(vm->stack, nil);
      }
      vm->pc += OpLength(op);
      break;
    case OpJump:
      vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      break;
    case OpReturn:
      vm->pc = RawInt(vm->regs[3]);
      break;
    case OpHalt:
      vm->halted = true;
      break;
    default:
      RuntimeError("Invalid op code", IntVal(op), vm);
      return;
    }
  }
}

void RuntimeError(char *message, Val exp, VM *vm)
{
  Print(IOFGRed);
  Print("(Runtime Error) ");
  PrintInt(vm->pc);
  Print(" ");
  Print(message);
  Print(": ");
  PrintVal(vm->mem, exp);
  Print(IOFGReset);
  Print("\n");
  vm->halted = true;
}
