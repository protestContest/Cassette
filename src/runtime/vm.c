#include "vm.h"
#include "native.h"
#include "ops.h"
#include "vec.h"
#include "io.h"

#define TRACE 1

// static void DebugPrompt(VM *vm);
// void PrintEnv(VM *vm);
// static bool DebugCmd(char *cmd, const char *name);
// static void PrintStack(VM *vm);
// static u32 PrintStackLine(VM *vm, u32 bufsize);
// static void PrintTraceHeader(void);
// static void PrintTraceStart(VM *vm);
// static void PrintTraceEnd(VM *vm);

static void Run(VM *vm);

void InitVM(VM *vm)
{
  vm->status = VM_Ok;
  vm->image = NULL;
  vm->stack = NULL;
  vm->module = NULL;
  vm->pc = 0;
  InitNativeMap(&vm->natives);
}

void ResetVM(VM *vm)
{
  FreeVec(vm->stack);
  InitVM(vm);
}

void RunImage(VM *vm, Image *image)
{
  InitVM(vm);
  vm->image = image;
  DefineNatives(vm);
  vm->module = GetModule(&image->modules, SymbolFor("Main"));
  Run(vm);
}

static void Run(VM *vm)
{
  while (vm->pc < VecCount(vm->module->code) && vm->status != VM_Halted) {
    // if (TRACE) PrintTraceStart(vm);

    DoOp(vm, ReadByte(vm));

    // if (TRACE) PrintTraceEnd(vm);

    // if (vm->status == VM_Debug || vm->status == VM_Error) {
    //   DebugPrompt(vm);
    // }
  }
}

void StackPush(VM *vm, Val val)
{
  VecPush(vm->stack, val);
}

Val StackPop(VM *vm)
{
  return VecPop(vm->stack);
}

Val StackPeek(VM *vm, u32 pos)
{
  return vm->stack[VecCount(vm->stack) - pos - 1];
}

void StackTrunc(VM *vm, u32 amount)
{
  RewindVec(vm->stack, amount);
}

u8 ReadByte(VM *vm)
{
  return vm->module->code[vm->pc++];
}

Val ReadConst(VM *vm)
{
  return GetConst(vm->module, ReadByte(vm));
}

void RuntimeError(VM *vm, char *msg)
{
  Print("Runtime error: ");
  Print(msg);
  Print("\n");

  ResetVM(vm);
  vm->status = VM_Error;
}

// void Interpret(VM *vm, char *src)
// {
//   Chunk chunk;
//   InitChunk(&chunk);

//   if (Compile(src, &chunk) == Error) {
//     ResetChunk(&chunk);
//     return;
//   }

//   if (VecCount(chunk.code) == 0) return;

//   Disassemble("Chunk", &chunk);

//   vm->chunk = &chunk;
//   vm->pc = 0;
//   Run(vm);
// }

// static void PrintTraceHeader(void)
// {
//   Print("─────┬╴Instruction╶─────────┬╴Stack╶───\n");
// }

// static void PrintTraceStart(VM *vm)
// {
//   u32 written = DisassembleInstruction(vm->chunk, vm->pc);
//   if (written < 30) {
//     for (u32 i = 0; i < 30 - written; i++) Print(" ");
//   }
//   Print("│ ");
//   fflush(stdout);
// }


// static void PrintTraceEnd(VM *vm)
// {
//   PrintStackLine(vm, 100);
//   Print(" e%d ", RawObj(vm->env));
//   PrintVMVal(vm, FrameMap(vm, vm->env));
//   if (!IsNil(ParentEnv(vm, vm->env))) {
//     Print(" <- e%d", RawObj(ParentEnv(vm, vm->env)));
//   }
//   Print("\n");
// }

// static u32 PrintStackLine(VM *vm, u32 bufsize)
// {
//   u32 written = 0;
//   for (u32 i = 0; i < VecCount(vm->stack); i++) {
//     if (written + 8 >= bufsize) {
//       if (i < VecCount(vm->stack) - 1) Print("...");
//       return written;
//     }

//     written += PrintVMVal(vm, vm->stack[VecCount(vm->stack)-1-i]);
//     written += Print(" ");
//   }
//   written += Print("▪︎");
//   return written;
// }
