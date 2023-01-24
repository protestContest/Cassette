#include "vm.h"
#include "chunk.h"
#include "primitives.h"
#include "printer.h"

const char *RegStr(Reg reg)
{
  switch (reg) {
  case REG_EXP:   return "EXP";
  case REG_ENV:   return "ENV";
  case REG_VAL:   return "VAL";
  case REG_CONT:  return "CONT";
  case REG_PROC:  return "PROC";
  case REG_ARGS:  return "ARGS";
  case REG_TMP:   return "TMP";
  default:        return "<reg>";
  }
}

#define STACK_SIZE 1024
VM *NewVM(void)
{
  VM *vm = malloc(sizeof(VM));
  vm->stack = malloc(STACK_SIZE);
  vm->chunk = NULL;
  for (u32 i = 0; i < NUM_REGS; i++) {
    vm->regs[i] = nil;
  }
  vm->sp = 0;
  vm->flag = false;
  vm->trace = false;
  return vm;
}

void OpMove(VM *vm)
{
  Reg src = ReadByte(vm->chunk);
  Reg dst = ReadByte(vm->chunk);
  vm->regs[dst] = vm->regs[src];
}

void OpAssign(VM *vm)
{
  Val val = ReadVal(vm->chunk);
  Reg reg = ReadByte(vm->chunk);
  vm->regs[reg] = val;
}

void OpTest(VM *vm)
{
  Val val = ReadVal(vm->chunk);
  Reg reg = ReadByte(vm->chunk);
  vm->flag = Eq(vm->regs[reg], val);
}

void OpBranch(VM *vm)
{
  Val loc = ReadVal(vm->chunk);
  if (!IsInt(loc)) {
    Error("Bad address: %s", ValStr(loc));
  }
  if (vm->flag) {
    vm->chunk->pos = RawInt(loc);
  }
}

void OpGoto(VM *vm)
{
  Val val = ReadVal(vm->chunk);
  if (!IsInt(val)) {
    Error("Bad address: %s", ValStr(val));
  }
  vm->chunk->pos = RawInt(val);
}

void OpSave(VM *vm)
{
  vm->stack[vm->sp++] = vm->regs[ReadByte(vm->chunk)];
}

void OpRestore(VM *vm)
{
  vm->regs[ReadByte(vm->chunk)] = vm->stack[--vm->sp];
}

void OpAdd(VM *vm)
{
  Val val = ReadVal(vm->chunk);
  Reg reg = ReadByte(vm->chunk);
  vm->regs[reg] = IntVal(RawInt(vm->regs[reg]) + RawInt(val));
}

void Trace(VM *vm)
{
  u32 pc = vm->chunk->pos;
  printf("%4u │ ", pc);
  u32 written = DisassembleInstruction(vm->chunk);
  vm->chunk->pos = pc;
  for (u32 i = 0; i < 30 - written; i++) printf(" ");
}

void PrintRegisters(VM *vm)
{
  printf("%c    ", vm->flag ? 'F' : 'f');
  for (u32 i = 0; i < NUM_REGS; i++) {
    printf("%s: %s    ", RegStr(i), ValStr(vm->regs[i]));
  }
}

void Step(VM *vm)
{
  if (vm->trace) Trace(vm);

  OpCode op = ReadByte(vm->chunk);
  switch (op) {
  case OP_LABEL:
    ReadVal(vm->chunk);
    break;
  case OP_MOVE:
    OpMove(vm);
    break;
  case OP_ASSIGN:
    OpAssign(vm);
    break;
  case OP_TEST:
    OpTest(vm);
    break;
  case OP_BRANCH:
    OpBranch(vm);
    break;
  case OP_GOTO:
    OpGoto(vm);
    break;
  case OP_SAVE:
    OpSave(vm);
    break;
  case OP_RESTORE:
    OpRestore(vm);
    break;
  case OP_ADD:
    OpAdd(vm);
    break;
  }

  if (vm->trace) {
    PrintRegisters(vm);
    printf("\n");
  }
}

void Run(VM *vm, Chunk *chunk)
{
  if (vm->trace) printf("━━╸VM Run╺━━\n");

  vm->chunk = chunk;
  chunk->pos = 0;
  for (u32 i = 0; i < NUM_REGS; i++) vm->regs[i] = nil;
  vm->flag = false;
  vm->sp = 0;
  while (vm->chunk->pos < vm->chunk->size) {
    Step(vm);
  }
}
