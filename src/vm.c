#pragma clang diagnostic ignored "-Wformat"
#include "vm.h"
#include "mem.h"
#include "printer.h"

OpStat op_stats[32] = {
  {"NOP                       ",      4,  1},
  {"CNST #%-2d, d%1d              ", 12,  3},
  {"MOVE d%1d, d%1d               ",  4,  3},
  {"LEA  $%02X, a%1d              ", 12,  3},
  {"CMP  d%1d, d%1d               ",  4,  3},
  {"CMPI #%1d, d%1d               ",  8,  3},
  {"BRA  %-+4d                 ",    10,  2},
  {"BEQ  %-+4d                 ",    12,  2}, // average time; depends on if branch is taken
  {"BNE  %-+4d                 ",    12,  2}, // average time; depends on if branch is taken
  {"BLT  %-+4d                 ",    12,  2}, // average time; depends on if branch is taken
  {"BLE  %-+4d                 ",    12,  2}, // average time; depends on if branch is taken
  {"BGT  %-+4d                 ",    12,  2}, // average time; depends on if branch is taken
  {"BGE  %-+4d                 ",    12,  2}, // average time; depends on if branch is taken
  {"SAVE d%1d                   ",    8,  2},
  {"RSTR d%1d                   ",    8,  2},
  {"BRK   Break               ",      4,  1},
  {"STOP                      ",      4,  1},
  {"SUB  d%1d                   ",    4,  3},
  {"EXG  d%1d %1d                 ",  6,  3},
};

Chunk *NewChunk(void)
{
  Chunk *chunk = malloc(sizeof(Chunk));
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->constants.capacity = 0;
  chunk->constants.count = 0;
  chunk->constants.values = NULL;
  return chunk;
}

void PutChunk(Chunk *chunk, u8 byte)
{
  if (chunk->count+1 > chunk->capacity) {
    chunk->capacity = (chunk->capacity == 0) ? 8 : 2*chunk->capacity;
    chunk->code = realloc(chunk->code, chunk->capacity);
    if (!chunk->code) Error("Memory error");
  }

  chunk->code[chunk->count++] = byte;
}

u8 PutConstant(Chunk *chunk, Val constant)
{
  if (chunk->constants.count+1 > chunk->constants.capacity) {
    chunk->constants.capacity = (chunk->constants.capacity == 0) ? 8 : 2*chunk->constants.capacity;
    chunk->constants.values = realloc(chunk->constants.values, sizeof(Val)*chunk->constants.capacity);
    if (!chunk->constants.values) Error("Memory error");
  }

  chunk->constants.values[chunk->constants.count++] = constant;
  return chunk->constants.count - 1;
}

Val ChunkConstant(Chunk *chunk, u8 index)
{
  return chunk->constants.values[index];
}

u32 PutInst(Chunk *chunk, OpCode op, ...)
{
  PutChunk(chunk, op);

  va_list args;
  u32 num_args = op_stats[op].size - 1;
  va_start(args, op);
  for (u32 i = 0; i < num_args; i++) {
    PutChunk(chunk, va_arg(args, u32));
  }
  va_end(args);

  return chunk->count;
}

u32 DisassembleInstruction(Chunk *chunk, u32 i)
{
  OpCode op = chunk->code[i];
  i8 arg[2];
  for (u32 j = 0; j < op_stats[op].size-1; j++) {
    arg[j] = chunk->code[i+1+j];
  }

  printf("│ %04d ", i);

  switch (op_stats[op].size) {
  case 1:
    printf(op_stats[op].name);
    break;
  case 2:
    printf(op_stats[op].name, arg[0]);
    break;
  case 3:
    printf(op_stats[op].name, arg[0], arg[1]);
    break;
  }
  printf(" │");

  return i + op_stats[op].size;
}

void Disassemble(Chunk *chunk, char *name)
{
  u32 len = strlen(name);
  printf("┌─╴%s╶", name);
  for (u32 i = 0; i < 30-len; i++) printf("─");
  printf("┐\n");
  for (u32 i = 0; i < chunk->count;) {
    i = DisassembleInstruction(chunk, i);
    printf("\n");
  }
  printf("└─────────────────────────────────┘\n");
}

void RunVM(VM *vm);

u8 ReadByte(VM *vm)
{
  return vm->chunk->code[vm->pc++];
}

Val ReadVal(VM *vm)
{
  Val val = *(Val*)(vm->chunk->code + vm->pc);
  vm->pc += sizeof(Val);
  return val;
}

VM *NewVM(void)
{
  VM *vm = malloc(sizeof(VM));
  vm->clock = 0;
  vm->carry = 0;
  vm->overflow = 0;
  vm->zero = 0;
  vm->negative = 0;
  vm->extend = 0;
  vm->pc = 0;
  for (u32 i = 0; i < 8; i++) vm->d[i] = nil;
  for (u32 i = 0; i < 8; i++) vm->a[i] = 0;
  vm->stack = malloc(sizeof(Val)*1024);
  vm->chunk = NULL;
  return vm;
}

void ExecuteChunk(VM *vm, Chunk *chunk)
{
  vm->pc = 0;
  vm->chunk = chunk;
  RunVM(vm);
}

bool IsTrue(Val value)
{
  return !IsNil(value) && !Eq(value, MakeSymbol("false", 5));
}

void PrintRegisters(VM *vm)
{
  printf("%c%c%c%c%c",
    vm->extend ? 'X' : 'x',
    vm->negative ? 'N' : 'n',
    vm->zero ? 'Z' : 'z',
    vm->overflow ? 'V' : 'v',
    vm->carry ? 'C' : 'c');

  printf(" │ ");
  for (u32 i = 0; i < 8; i++) {
    printf("%4d ", vm->a[i]);
  }
  printf(" │ ");
  for (u32 i = 0; i < 8; i++) {
    DebugVal(vm->d[i]);
    printf(" ");
  }
  printf("\n");
}

void RunVM(VM *vm)
{
  i8 arg[2];
  i32 op_result;
  i8 op_convert;
  Val tmp;
  vm->clock = 0;
  bool step = false;
  bool branched = false;

  printf("┌──────────────────────────╴Run╶──┬────────┬──────────────────────────────────────────┬╴\n");

  while (true) {
    branched = false;

    DisassembleInstruction(vm->chunk, vm->pc);

    OpCode op = ReadByte(vm);
    for (u32 j = 0; j < op_stats[op].size-1; j++) {
      arg[j] = (i8)ReadByte(vm);
    }

    vm->clock += op_stats[op].time;

    switch (op) {
    case OP_NOP:
      break;

    case OP_CNST:
      vm->d[arg[1]] = vm->chunk->constants.values[arg[0]];
      break;

    case OP_MOVE:
      vm->d[arg[1]] = vm->d[arg[0]];
      break;

    case OP_LEA:
      vm->a[arg[1]] = arg[0];
      break;

    case OP_CMP:
      op_result = RawVal(vm->d[arg[1]]) - RawVal(vm->d[arg[0]]);
      vm->negative = op_result < 0;
      vm->zero = op_result == 0;
      vm->overflow = op_result < -32768;
      vm->carry = RawVal(vm->d[arg[1]]) < RawVal(vm->d[arg[0]]);
      break;

    case OP_CMPI:
      op_result = (i32)RawVal(vm->d[arg[1]]) - (i32)arg[0];
      vm->negative = op_result < 0;
      vm->zero = op_result == 0;
      vm->overflow = op_result < -32768;
      vm->carry = RawVal(vm->d[arg[1]]) < arg[0];
      break;

    case OP_BRA:
      vm->pc += (i32)arg[0];
      branched = true;
      break;

    case OP_BEQ:
      if (vm->zero) {
        vm->pc += (i32)arg[0];
        branched = true;
      }
      break;

    case OP_BNE:
      if (!vm->zero) {
        vm->pc += (i32)arg[0];
        branched = true;
      }
      break;

    case OP_BLT:
      if (vm->negative != vm->overflow) {
        vm->pc += (i32)arg[0];
        branched = true;
      }
      break;

    case OP_BLE:
      if (vm->zero || (vm->negative != vm->overflow)) {
        vm->pc += (i32)arg[0];
        branched = true;
      }
      break;

    case OP_BGT:
      if (!vm->zero && (vm->negative == vm->overflow)) {
        vm->pc += (i32)arg[0];
        branched = true;
      }
      break;

    case OP_BGE:
      op_convert = arg[0];
      if (vm->negative == vm->overflow) {
        vm->pc += op_convert;
        branched = true;
      }
      break;

    case OP_SAVE:
      vm->stack[vm->SP++] = vm->d[arg[0]];
      break;

    case OP_RSTR:
      vm->d[arg[0]] = vm->stack[--vm->SP];
      break;

    case OP_EXG:
      tmp = vm->d[arg[0]];
      vm->d[arg[0]] = vm->d[arg[1]];
      vm->d[arg[1]] = tmp;
      break;

    case OP_BRK:
      step = true;
      break;

    case OP_STOP:
      printf("  ");
      PrintRegisters(vm);
      printf("└─╴Time: %4d╶─────────────╴Halt╶─┴────────┴──────────────────────────────────────────┴╴\n", vm->clock);
      return;

    case OP_SUB:
      op_result = (i8)RawVal(vm->d[arg[1]]) - (i8)RawVal(vm->d[arg[0]]);
      vm->negative = op_result < 0;
      vm->zero = op_result == 0;
      vm->overflow = op_result < -32768;
      vm->carry = RawVal(vm->d[arg[1]]) < RawVal(vm->d[arg[0]]);
      vm->d[arg[1]] = IntVal(op_result);
      break;
    }

    printf(" %c", branched ? '!' : ' ');

    PrintRegisters(vm);

    if (step) {
      int c = getchar();
      printf("\x1B[1A");
      switch (c) {
      case 'c':
        step = false;
        break;
      }
    }
  }
}
