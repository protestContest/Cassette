#include "vm.h"
#include "native.h"
#include "ops.h"
#include "mem.h"
#include <univ/vec.h>
#include <univ/io.h>
#include "print.h"
#include "env.h"

#define TRACE

static void DebugPrompt(VM *vm);
static u32 PrintStackLine(VM *vm, u32 bufsize);
static void PrintTraceHeader(void);
static void PrintTraceStart(VM *vm);
static void PrintTraceEnd(VM *vm);

static void Run(VM *vm);

void InitVM(VM *vm)
{
  vm->status = VM_Ok;
  vm->stack = NULL;
  InitMem(&vm->heap);
  vm->symbols = MakeMap(&vm->heap, 32);
  vm->chunk = NULL;
  vm->pc = 0;
  InitNativeMap(&vm->natives);
}

void RunChunk(VM *vm, Chunk *chunk)
{
  vm->chunk = chunk;
  DefineNatives(vm);
  Run(vm);
}

static void Run(VM *vm)
{
#ifdef TRACE
  PrintTraceHeader();
#endif

  while (vm->pc < VecCount(vm->chunk->code) && vm->status == VM_Ok) {
#ifdef TRACE
    PrintTraceStart(vm);
#endif

    DoOp(vm, ReadByte(vm));

#ifdef TRACE
    PrintTraceEnd(vm);
#endif
  }
}

void StackPush(VM *vm, Val val)
{
  VecPush(vm->stack, val);
}

Val StackPop(VM *vm)
{
  return VecPop(vm->stack, nil);
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
  return vm->chunk->code[vm->pc++];
}

Val ReadConst(VM *vm)
{
  return GetConst(vm->chunk, ReadByte(vm));
}

void RuntimeError(VM *vm, char *msg)
{
  Print("Runtime error: ");
  Print(msg);
  Print("\n");
  vm->status = VM_Error;
}

static void PrintTraceHeader(void)
{
  Print("─────┬╴Instruction╶─────────┬╴Stack╶───\n");
}

static void PrintTraceStart(VM *vm)
{
  u32 written = DisassembleInstruction(vm->chunk, vm->pc);
  if (written < 30) {
    for (u32 i = 0; i < 30 - written; i++) Print(" ");
  }
  Print("│ ");
}


static void PrintTraceEnd(VM *vm)
{
  PrintStackLine(vm, 100);
  Print(" e");
  PrintInt(RawVal(vm->env), 3);
  Print(" ");
  PrintVal(vm->heap, vm->symbols, FrameMap(vm->heap, vm->env));
  if (!IsNil(ParentEnv(vm->heap, vm->env))) {
    Print(" <- e");
    PrintInt(RawVal(ParentEnv(vm->heap, vm->env)), 3);
  }
  Print("\n");
  Flush(output);
}

static u32 PrintStackLine(VM *vm, u32 bufsize)
{
  u32 written = 0;
  for (u32 i = 0; i < VecCount(vm->stack); i++) {
    if (written + 8 >= bufsize) {
      if (i < VecCount(vm->stack) - 1) Print("...");
      return written;
    }

    written += PrintVal(vm->heap, vm->symbols, vm->stack[VecCount(vm->stack)-1-i]);
    Print(" ");
    written++;
  }
  Print("▪︎");
  written++;
  return written;
}
