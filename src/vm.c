#include "vm.h"
#include "chunk.h"
#include "compile.h"
#include "ops.h"
#include "vec.h"
#include "printer.h"
#include "mem.h"

#define TRACE 1

void AddFrame(VM *vm)
{
  vm->env = MakePair(&vm->heap, nil, vm->env);
}

void InitVM(VM *vm)
{
  vm->status = Ok;
  vm->pc = 0;
  vm->chunk = NULL;
  vm->stack = NULL;
  vm->heap = NULL;
  vm->env = nil;
  VecPush(vm->heap, nil);
  VecPush(vm->heap, nil);

  AddFrame(vm);
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

    written += PrintVal(vm->heap, vm->stack[VecCount(vm->stack)-1-i]);
    written += printf(" ");
  }
  printf("▪︎");
}

Status Run(VM *vm)
{
  u32 count = 0;

  if (TRACE) printf("───╴Line╶┬╴Instruction╶─────┬╴Stack╶──\n");

  while (vm->pc < VecCount(vm->chunk->code) && vm->status == Ok) {
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
    case OP_HALT:
      vm->status = Halted;
      break;
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
    case OP_GT:
    case OP_LT:
      CompareOp(vm, op);
      break;
    case OP_LOOKUP:
      break;
    case OP_PAIR:
      PairOp(vm);
      break;
    case OP_LIST:
      ListOp(vm, ReadByte(vm));
      break;
    case OP_JUMP:
      vm->pc += (i8)ReadByte(vm);
      break;
    case OP_LAMBDA:
      LambdaOp(vm);
      break;
    case OP_APPLY:
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

  if (TRACE) {
    PrintHeap(vm->heap);
  }

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
