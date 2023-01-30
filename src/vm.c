#include "vm.h"
#include "chunk.h"
#include "compile.h"
#include "ops.h"
#include "vec.h"
#include "printer.h"
#include "mem.h"

#define TRACE 1

void InitVM(VM *vm)
{
  vm->status = Ok;
  vm->pc = 0;
  vm->chunk = NULL;
  vm->stack = NULL;
}

void ResetStack(VM *vm)
{
  FreeVec(vm->stack);
  vm->stack = NULL;
}

void FreeVM(VM *vm)
{
  FreeVec(vm->stack);
}

static u8 ReadByte(VM *vm)
{
  return vm->chunk->code[vm->pc++];
}

static Val ReadConst(VM *vm)
{
  return GetConst(vm->chunk, ReadByte(vm));
}

static void PrintStack(VM *vm, u32 bufsize)
{
  u32 written = 0;
  for (u32 i = 0; i < VecCount(vm->stack); i++) {
    if (written + 8 >= bufsize) {
      if (i < VecCount(vm->stack) - 1) printf("...");
      return;
    }

    written += PrintVal(vm->chunk->heap, vm->stack[VecCount(vm->stack)-1-i]);
    written += printf("  ");
  }
  printf("▪︎");
}

Status Run(VM *vm)
{
  u32 count = 0;

  if (TRACE) printf("───╴Line╶┬╴Instruction╶─────┬╴Stack╶──\n");

  while (vm->pc < VecCount(vm->chunk->code)) {
    count++;
    if (TRACE) {
      if (count % 30 == 0) printf("─────────┼╴Instruction╶─────┼╴Stack╶──\n");
      u32 written = DisassembleInstruction(vm->chunk, vm->pc);
      for (u32 i = 0; i < 30 - written; i++) printf(" ");
      printf("│ ");
      PrintStack(vm, 100);
      printf("\n");
    }

    OpCode op;
    switch (op = ReadByte(vm)) {
    case OP_RETURN:
      return Ok;
    case OP_CONST:
      VecPush(vm->stack, ReadConst(vm));
      break;
    case OP_NEG:
      NegOp(vm);
      break;
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
    case OP_EXP:
      ArithmeticOp(vm, op);
      break;
    case OP_TRUE:
      VecPush(vm->stack, SymbolFor("true"));
      break;
    case OP_FALSE:
      VecPush(vm->stack, SymbolFor("false"));
      break;
    case OP_NIL:
      VecPush(vm->stack, nil);
      break;
    case OP_NOT:
      NotOp(vm);
      break;
    case OP_EQUAL:
      break;
    case OP_GT:
      break;
    case OP_LT:
      break;
    case OP_CONS:
      break;
    }
  }

  if (TRACE) printf("\n");

  return Ok;
}

Status Interpret(VM *vm, char *src)
{
  Chunk chunk;
  InitChunk(&chunk);

  if (Compile(src, &chunk) == Error) {
    ResetChunk(&chunk);
    return Error;
  }

  Disassemble("Chunk", &chunk);

  vm->chunk = &chunk;
  vm->pc = 0;
  Status result = Run(vm);

  ResetStack(vm);

  return result;
}

void RuntimeError(VM *vm, char *fmt, ...)
{
  u32 line = vm->chunk->lines[2*vm->pc];
  u32 col = vm->chunk->lines[2*vm->pc+1];

  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "[%d:%d] Runtime error: ", line, col);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");

  ResetStack(vm);
  vm->status = Error;
}


// #define STACK_SIZE 1024
// // VM *NewVM(void)
// {
//   VM *vm = malloc(sizeof(VM));
//   vm->stack = malloc(STACK_SIZE);
//   vm->chunk = NULL;
//   for (u32 i = 0; i < NUM_REGS; i++) {
//     vm->regs[i] = nil;
//   }
//   vm->sp = 0;
//   vm->flag = false;
//   vm->trace = false;
//   return vm;
// }

// void OpMove(VM *vm)
// {
//   Reg src = ReadByte(vm->chunk);
//   Reg dst = ReadByte(vm->chunk);
//   vm->regs[dst] = vm->regs[src];
// }

// void OpAssign(VM *vm)
// {
//   Val val = ReadVal(vm->chunk);
//   Reg reg = ReadByte(vm->chunk);
//   vm->regs[reg] = val;
// }

// void OpTest(VM *vm)
// {
//   Val val = ReadVal(vm->chunk);
//   Reg reg = ReadByte(vm->chunk);
//   vm->flag = Eq(vm->regs[reg], val);
// }

// void OpBranch(VM *vm)
// {
//   Val loc = ReadVal(vm->chunk);
//   if (!IsInt(loc)) {
//     Error("Bad address: %s", ValStr(loc));
//   }
//   if (vm->flag) {
//     vm->chunk->pos = RawInt(loc);
//   }
// }

// void OpGoto(VM *vm)
// {
//   Val val = ReadVal(vm->chunk);
//   if (!IsInt(val)) {
//     Error("Bad address: %s", ValStr(val));
//   }
//   vm->chunk->pos = RawInt(val);
// }

// void OpSave(VM *vm)
// {
//   vm->stack[vm->sp++] = vm->regs[ReadByte(vm->chunk)];
// }

// void OpRestore(VM *vm)
// {
//   vm->regs[ReadByte(vm->chunk)] = vm->stack[--vm->sp];
// }

// void OpAdd(VM *vm)
// {
//   Val val = ReadVal(vm->chunk);
//   Reg reg = ReadByte(vm->chunk);
//   vm->regs[reg] = IntVal(RawInt(vm->regs[reg]) + RawInt(val));
// }

// void Trace(VM *vm)
// {
//   u32 pc = vm->chunk->pos;
//   printf("%4u │ ", pc);
//   u32 written = DisassembleInstruction(vm->chunk);
//   vm->chunk->pos = pc;
//   for (u32 i = 0; i < 30 - written; i++) printf(" ");
// }

// void PrintRegisters(VM *vm)
// {
//   printf("%c    ", vm->flag ? 'F' : 'f');
//   for (u32 i = 0; i < NUM_REGS; i++) {
//     printf("%s: %s    ", RegStr(i), ValStr(vm->regs[i]));
//   }
// }

// void Step(VM *vm)
// {
//   if (vm->trace) Trace(vm);

//   OpCode op = ReadByte(vm->chunk);
//   switch (op) {
//   case OP_LABEL:
//     ReadVal(vm->chunk);
//     break;
//   case OP_MOVE:
//     OpMove(vm);
//     break;
//   case OP_ASSIGN:
//     OpAssign(vm);
//     break;
//   case OP_TEST:
//     OpTest(vm);
//     break;
//   case OP_BRANCH:
//     OpBranch(vm);
//     break;
//   case OP_GOTO:
//     OpGoto(vm);
//     break;
//   case OP_SAVE:
//     OpSave(vm);
//     break;
//   case OP_RESTORE:
//     OpRestore(vm);
//     break;
//   case OP_ADD:
//     OpAdd(vm);
//     break;
//   }

//   if (vm->trace) {
//     PrintRegisters(vm);
//     printf("\n");
//   }
// }

// void Run(VM *vm, Chunk *chunk)
// {
//   if (vm->trace) printf("━━╸VM Run╺━━\n");

//   vm->chunk = chunk;
//   chunk->pos = 0;
//   for (u32 i = 0; i < NUM_REGS; i++) vm->regs[i] = nil;
//   vm->flag = false;
//   vm->sp = 0;
//   while (vm->chunk->pos < vm->chunk->size) {
//     Step(vm);
//   }
// }
