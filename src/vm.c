#include "vm.h"
#include "chunk.h"
#include "compile.h"
#include "ops.h"
#include "vec.h"
#include "printer.h"
#include "mem.h"
#include "env.h"

#define TRACE 1

void InitVM(VM *vm)
{
  vm->status = VM_Ok;
  vm->pc = 0;
  vm->chunk = NULL;
  vm->stack = NULL;
  vm->heap = NULL;
  VecPush(vm->heap, nil);
  VecPush(vm->heap, nil);
  vm->env = ExtendEnv(vm, nil);
}

void ResetVM(VM *vm)
{
  vm->status = VM_Ok;
  FreeVec(vm->stack);
  vm->stack = NULL;
  FreeVec(vm->heap);
  vm->heap = NULL;
  VecPush(vm->heap, nil);
  VecPush(vm->heap, nil);
  vm->env = nil;
  vm->pc = 0;

  if (vm->chunk) {
    vm->heap = AppendVec(vm->heap, vm->chunk->constants, sizeof(Val));
    vm->env = ExtendEnv(vm, vm->env);
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
  return vm->chunk->code[vm->pc++];
}

Val ReadConst(VM *vm)
{
  return GetConst(vm->chunk, ReadByte(vm));
}

static void PrintStackLine(VM *vm, u32 bufsize)
{
  u32 written = 0;
  for (u32 i = 0; i < VecCount(vm->stack); i++) {
    if (written + 8 >= bufsize) {
      if (i < VecCount(vm->stack) - 1) printf("...");
      return;
    }

    written += PrintVal(vm->heap, vm->chunk->symbols, vm->stack[VecCount(vm->stack)-1-i]);
    written += printf(" ");
  }
  printf("▪︎");
}

static void PrintStack(VM *vm)
{
  printf("───╴Stack╶───\n");
  if (VecCount(vm->stack) == 0) {
    printf("(empty)\n");
    return;
  }

  for (u32 i = 0; i < VecCount(vm->stack) && i < 100; i++) {
    printf("%4u │ ", i);
    PrintVal(vm->heap, vm->chunk->symbols, vm->stack[i]);
    printf("\n");
  }
}

static bool DebugCmd(char *cmd, const char *name)
{
  for (u32 i = 0; i < strlen(name); i++) {
    if (cmd[i] != name[i]) {
      return false;
    }
  }

  return true;
}

void PrintEnv(VM *vm)
{
  printf("─────╴Environment╶──────\n");
  if (IsNil(vm->env)) {
    printf("(empty)\n");
    return;
  }

  Val env = vm->env;
  u32 frame_num = 0;
  while (!IsNil(env)) {
    Val frame = Head(vm->heap, env);
    while (!IsNil(frame)) {
      Val pair = Head(vm->heap, frame);
      Val var = Head(vm->heap, pair);
      Val val = Tail(vm->heap, pair);

      i32 written = PrintVal(vm->heap, vm->chunk->symbols, var);
      for (i32 i = 0; i < 10 - written; i++) printf(" ");
      printf("│ ");
      PrintVal(vm->heap, vm->chunk->symbols, val);
      printf("\n");

      frame = Tail(vm->heap, frame);
    }
    frame_num++;
    env = Tail(vm->heap, env);
    printf("────────────────────────\n");
  }
}

void DebugVM(VM *vm)
{
  char line[1024];
  while (true) {
    printf("? ");
    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      vm->status = VM_Ok;
      return;
    }

    if (DebugCmd(line, "c")) {
      vm->status = VM_Ok;
      printf("\n");
      return;
    } else if (DebugCmd(line, "help")) {
      printf("c       continue\nst      stack dump\nhd      heap dump\nsm      symbol dump\nr       reset/run\np       print chunk\ne       dump environment\n[enter] step\n");
    } else if (DebugCmd(line, "st")) {
      PrintStack(vm);
    } else if (DebugCmd(line, "hd")) {
      PrintHeap(vm->heap, vm->chunk->symbols);
    } else if (DebugCmd(line, "sm")) {
      DumpSymbols(vm->chunk->symbols);
    } else if (DebugCmd(line, "r")) {
      ResetVM(vm);
      return;
    } else if (DebugCmd(line, "p")) {
      Disassemble("Chunk", vm->chunk);
    } else if (DebugCmd(line, "e")) {
      PrintEnv(vm);
    } else {
      return;
    }
  }
}

void Run(VM *vm)
{
  u32 count = 0;

  if (TRACE) printf("─────┬╴Instruction╶─────────┬╴Stack╶───\n");

  while (vm->pc < VecCount(vm->chunk->code)) {
    count++;
    if (TRACE) {
      if (count % 30 == 0) printf("─────┼╴Instruction╶─────────┼╴Stack╶───\n");
      u32 written = DisassembleInstruction(vm->chunk, vm->pc);
      for (u32 i = 0; i < 30 - written; i++) printf(" ");
      printf("│ ");
    }

    DoOp(vm, ReadByte(vm));

    if (TRACE) {
      PrintStackLine(vm, 100);
      printf("\n");
    }

    if (vm->status == VM_Debug || vm->status == VM_Error) {
      DebugVM(vm);
    }

    if (count >= 100) exit(1);
  }
}

void RunChunk(VM *vm, Chunk *chunk)
{
  vm->chunk = chunk;
  ResetVM(vm);
  Run(vm);
}

void Interpret(VM *vm, char *src)
{
  Chunk chunk;
  InitChunk(&chunk);

  if (Compile(src, &chunk) == Error) {
    ResetChunk(&chunk);
    return;
  }

  Disassemble("Chunk", &chunk);

  vm->chunk = &chunk;
  vm->pc = 0;
  Run(vm);
}

void RuntimeError(VM *vm, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "Runtime error: ");
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");

  ResetVM(vm);
  vm->status = VM_Error;
}
