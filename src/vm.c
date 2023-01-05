#include "vm.h"
#include "mem.h"
#include "printer.h"

char *RegName(Register reg)
{
  switch (reg) {
  case REG_A:   return "A";
  case REG_B:   return "B";
  case REG_T:   return "T";
  case REG_Z:   return "Z";
  case REG_ADDR:  return "ADDR";
  }
}

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

void Assign(Chunk *chunk, u8 constant, Register reg)
{
  PutChunk(chunk, OP_ASSIGN);
  PutChunk(chunk, constant);
  PutChunk(chunk, reg);
}

void Move(Chunk *chunk, Register src, Register dst)
{
  PutChunk(chunk, OP_MOVE);
  PutChunk(chunk, src);
  PutChunk(chunk, dst);
}

void Test(Chunk *chunk, Register reg)
{
  PutChunk(chunk, OP_TEST);
  PutChunk(chunk, reg);
}

void Compare(Chunk *chunk, Register a, Register b)
{
  PutChunk(chunk, OP_CMP);
  PutChunk(chunk, a);
  PutChunk(chunk, b);
}

void Branch(Chunk *chunk, Register reg)
{
  PutChunk(chunk, OP_BRANCH);
  PutChunk(chunk, reg);
}

void Goto(Chunk *chunk, Register reg)
{
  PutChunk(chunk, OP_GOTO);
  PutChunk(chunk, reg);
}

void Save(Chunk *chunk, Register reg)
{
  PutChunk(chunk, OP_SAVE);
  PutChunk(chunk, reg);
}

void Restore(Chunk *chunk, Register reg)
{
  PutChunk(chunk, OP_RESTORE);
  PutChunk(chunk, reg);
}

void Perform(Chunk *chunk)
{
  PutChunk(chunk, OP_PERFORM);
}

void Halt(Chunk *chunk)
{
  PutChunk(chunk, OP_HALT);
}

void Break(Chunk *chunk)
{
  PutChunk(chunk, OP_BREAK);
}

void Rem(Chunk *chunk, Register a, Register b, Register dst)
{
  PutChunk(chunk, OP_REM);
  PutChunk(chunk, a);
  PutChunk(chunk, b);
  PutChunk(chunk, dst);
}

u32 DisassembleInstruction(Chunk *chunk, u32 i)
{
  printf("│ %04d ", i);
  switch (chunk->code[i]) {
  case OP_ASSIGN:
    printf("ASSN %8s %4s ", ValStr(ChunkConstant(chunk, chunk->code[i+1])), RegName(chunk->code[i+2]));
    return i+3;

  case OP_MOVE:
    printf("MOVE %4s %4s     ", RegName(chunk->code[i+1]), RegName(chunk->code[i+2]));
    return i+3;

  case OP_TEST:
    printf("TEST %4s          ", RegName(chunk->code[i+1]));
    return i+2;

  case OP_BRANCH:
    printf("BRCH %4s          ", RegName(chunk->code[i+1]));
    return i+2;

  case OP_GOTO:
    printf("GOTO %4s          ", RegName(chunk->code[i+1]));
    return i+2;

  case OP_SAVE:
    printf("SAVE %4s          ", RegName(chunk->code[i+1]));
    return i+2;

  case OP_RESTORE:
    printf("RSTR %4s          ", RegName(chunk->code[i+1]));
    return i+2;

  case OP_PERFORM:
    printf("PFRM               ");
    return i+1;

  case OP_HALT:
    printf("HALT               ");
    return i+1;

  case OP_BREAK:
    printf("BREAK              ");
    return i+1;

  case OP_REM:
    printf("REM  %4s %4s %4s", RegName(chunk->code[i+1]), RegName(chunk->code[i+2]), RegName(chunk->code[i+3]));
    return i+4;

  case OP_CMP:
    printf("CMP  %4s %4s     ", RegName(chunk->code[i+1]), RegName(chunk->code[i+2]));
    return i+3;

  default:
    printf("Unknown instruction: %02X", chunk->code[i]);
    return i+1;
  }
}

void Disassemble(Chunk *chunk)
{
  printf("┌─────────────────────────────\n");
  for (u32 i = 0; i < chunk->count;) {
    i = DisassembleInstruction(chunk, i);
    printf("\n");
  }
  printf("└─────────────────────────────\n");
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

void RunVM(VM *vm)
{
  while (true) {
    // getchar();
    DisassembleInstruction(vm->chunk, vm->pc);
    switch (ReadByte(vm)) {
    case OP_ASSIGN: {
      u8 src = ReadByte(vm);
      Register dst = ReadByte(vm);
      vm->registers[dst] = ChunkConstant(vm->chunk, src);
      break;
    }

    case OP_MOVE: {
      Register src = ReadByte(vm);
      Register dst = ReadByte(vm);
      vm->registers[dst] = vm->registers[src];
      break;
    }

    case OP_TEST:
      vm->flag = IsTrue(vm->registers[ReadByte(vm)]);
      break;

    case OP_BRANCH:
      if (vm->flag) {
        Register reg = ReadByte(vm);
        vm->pc = RawVal(vm->registers[reg]);
      } else {
        vm->pc++;
      }
      break;

    case OP_GOTO: {
      u8 byte = ReadByte(vm);
      vm->pc = RawVal(vm->registers[byte]);
      break;
    }

    case OP_SAVE:
      vm->stack[vm->sp++] = vm->registers[ReadByte(vm)];
      break;

    case OP_RESTORE:
      vm->registers[ReadByte(vm)] = vm->stack[--vm->sp];
      break;

    case OP_PERFORM:
      Error("I don't know what this does!");
      break;

    case OP_HALT:
      return;

    case OP_BREAK:
      getchar();
      break;

    case OP_REM: {
      Val a = vm->registers[ReadByte(vm)];
      Val b = vm->registers[ReadByte(vm)];
      vm->registers[ReadByte(vm)] = IntVal((u32)RawVal(a) % (u32)RawVal(b));
      break;
    }
    case OP_CMP:
      vm->flag = Eq(vm->registers[ReadByte(vm)], vm->registers[ReadByte(vm)]);
      break;
    }

    printf("    A: %d B: %d T: %d Z: %d ADDR: %d F: %c\n",
      (u32)RawVal(vm->registers[REG_A]),
      (u32)RawVal(vm->registers[REG_B]),
      (u32)RawVal(vm->registers[REG_T]),
      (u32)RawVal(vm->registers[REG_Z]),
      (u32)RawVal(vm->registers[REG_ADDR]),
      (vm->flag) ? '+' : '-');
  }
}

void DumpVM(VM *vm)
{
  printf("Flag: %d\n", vm->flag);
  printf("PC: %d\n", vm->pc);
  printf("Registers:\n");
  for (u32 i = 0; i < 16; i++) {
    printf("  ");
    PrintVal(vm->registers[i]);
  }
}
