#include "runtime/vm.h"
#include "runtime/mem.h"
#include "runtime/ops.h"
#include "runtime/symbol.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/vec.h"

#define VMDone(vm) ((vm)->error || (vm)->pc >= VecCount((vm)->program->code))

void InitVM(VM *vm, Program *program)
{
  u32 i;
  vm->error = 0;
  vm->pc = 0;
  for (i = 0; i < ArrayCount(vm->regs); i++) vm->regs[i] = 0;
  vm->link = 0;
  vm->program = program;
  SetSymbolSize(valBits);
  if (program) {
    char *names = program->strings;
    u32 len = VecCount(program->strings);
    char *end = names + len;
    while (names < end) {
      len = StrLen(names);
      SymbolFrom(names, len);
      names += len + 1;
    }
  }
  vm->refs = 0;
}

void DestroyVM(VM *vm)
{
  FreeVec(vm->refs);
}

void VMStep(VM *vm)
{
  ExecOp(vm->program->code[vm->pc], vm);
}

static u32 PrintStack(VM *vm, u32 max)
{
  u32 i, printed = 0;
  char *str;
  for (i = 0; i < max; i++) {
    if (i >= StackSize()) break;
    str = MemValStr(StackPeek(i));
    printed += fprintf(stderr, "%s ", str);
    free(str);
  }
  if (StackSize() > max) {
    printed += fprintf(stderr, "... [%d]", StackSize() - max);
  }
  for (i = 0; (i32)i < 30 - (i32)printed; i++) fprintf(stderr, " ");
  return printed;
}

static void VMTrace(VM *vm)
{
  u32 index = vm->pc;
  u32 i, len;
  len = DisassembleInst(vm->program->code, &index);
  if (len >= 20) {
    fprintf(stderr, "\n");
    len = 0;
  }
  for (i = 0; i < 20 - len; i++) fprintf(stderr, " ");

  PrintStack(vm, 20);
  fprintf(stderr, "\n");
}

Error *VMRun(Program *program, bool trace)
{
  VM vm;

  InitVM(&vm, program);
  InitMem(256);

  if (trace) {
    u32 num_width = NumDigits(VecCount(program->code), 10);
    u32 i;
    for (i = 0; i < num_width; i++) fprintf(stderr, "─");
    fprintf(stderr, "┬─inst─────────stack───────────────\n");
    while (!VMDone(&vm)) {
      VMTrace(&vm);
      VMStep(&vm);
    }
    for (i = 0; i < 20; i++) fprintf(stderr, " ");
    PrintStack(&vm, 20);
    fprintf(stderr, "\n");
  } else {
    while (!VMDone(&vm)) VMStep(&vm);
  }

  DestroyVM(&vm);
  return vm.error;
}

u32 VMPushRef(void *ref, VM *vm)
{
  u32 index = VecCount(vm->refs);
  VecPush(vm->refs, ref);
  return index;
}

void *VMGetRef(u32 ref, VM *vm)
{
  return vm->refs[ref];
}

i32 VMFindRef(void *ref, VM *vm)
{
  u32 i;
  for (i = 0; i < VecCount(vm->refs); i++) {
    if (vm->refs[i] == ref) return i;
  }
  return -1;
}

static void RunGC(VM *vm)
{
  CollectGarbage(vm->regs, ArrayCount(vm->regs));
}

void MaybeGC(u32 size, VM *vm)
{
  if (MemFree() < size) RunGC(vm);
  if (MemFree() < size) {
    u32 needed = MemCapacity() + size - MemFree();
    SizeMem(Max(2*MemCapacity(), needed));
  }
  assert(MemFree() >= size);
}

u32 RuntimeError(char *message, VM *vm)
{
  vm->error = NewRuntimeError(message, vm->pc, vm->link, &vm->program->srcmap);
  return 0;
}
