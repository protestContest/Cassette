#include "module.h"
#include "vec.h"

Module *CreateModule(void)
{
  Module *module = malloc(sizeof(Module));
  module->code = NULL;
  module->constants = NULL;
  module->env = nil;
  return module;
}

u32 ChunkSize(Module *module)
{
  return VecCount(module->code);
}

u32 PutByte(Module *module, u8 byte)
{
  VecPush(module->code, byte);
  return VecCount(module->code) - 1;
}

void SetByte(Module *module, u32 i, u8 byte)
{
  module->code[i] = byte;
}

u8 GetByte(Module *module, u32 i)
{
  return module->code[i];
}

u8 PutConst(Module *module, Val value)
{
  for (u32 i = 0; i < VecCount(module->constants); i++) {
    if (Eq(module->constants[i], value)) return i;
  }

  VecPush(module->constants, value);
  return VecCount(module->constants) - 1;
}

Val GetConst(Module *module, u32 i)
{
  return module->constants[i];
}

u32 PutInst(Module *module, OpCode op, ...)
{
  va_list args;
  va_start(args, op);

#if DEBUG_COMPILE
  u32 i = ChunkSize(module);
#endif

  PutByte(module, op);

  if (op == OP_CONST) {
    PutByte(module, PutConst(module, va_arg(args, Val)));
  } else if (OpSize(op) > 1) {
    PutByte(module, va_arg(args, u32));
  }

#if DEBUG_COMPILE
  DisassembleInstruction(module, i);
  printf("\n");
#endif

  va_end(args);
  return VecCount(module->code) - OpSize(op);
}

// u32 DisassembleInstruction(Chunk *chunk, u32 i)
// {
//   u32 written = printf("%4u │ ", i);

//   OpCode op = chunk->code[i];
//   switch (OpFormat(op)) {
//   case ARGS_VAL:
//     written += printf("%02X %02X %s ", GetByte(chunk, i), GetByte(chunk, i+1), OpStr(op));
//     written += PrintVal(chunk->constants, &chunk->strings, GetConst(chunk, GetByte(chunk, i + 1)));
//     break;
//   case ARGS_INT:
//     written += printf("%02X %02X %s %d", GetByte(chunk, i), GetByte(chunk, i+1), OpStr(op), GetByte(chunk, i + 1));
//     break;
//   default:
//     written += printf("%02X    %s", GetByte(chunk, i), OpStr(op));
//     break;
//   }

//   return written;
// }

// void Disassemble(char *title, Chunk *chunk)
// {
//   printf("───╴%s╶───\n", title);

//   for (u32 i = 0; i < VecCount(chunk->code); i += OpSize(chunk->code[i])) {
//     DisassembleInstruction(chunk, i);
//     printf("\n");
//   }
//   printf("\n");
// }








