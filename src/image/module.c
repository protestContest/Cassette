#include "module.h"
#include "../vec.h"
#include "../platform/allocate.h"
#include "../platform/console.h"
#include "../runtime/print.h"
#include <stdarg.h>

Module *CreateModule(void)
{
  Module *module = Allocate(sizeof(Module));
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

  PutByte(module, op);

  if (op == OP_CONST) {
    PutByte(module, PutConst(module, va_arg(args, Val)));
  } else if (OpSize(op) > 1) {
    PutByte(module, va_arg(args, u32));
  }

  va_end(args);
  return VecCount(module->code) - OpSize(op);
}

u32 DisassembleInstruction(Module *module, StringMap *strings, u32 i)
{
  u32 written = Print("%4u │ ", i);

  OpCode op = module->code[i];
  switch (OpFormat(op)) {
  case ARGS_VAL:
    written += Print("%02X %02X %s ", GetByte(module, i), GetByte(module, i+1), OpStr(op));
    written += PrintVal(module->constants, strings, GetConst(module, GetByte(module, i + 1)));
    break;
  case ARGS_INT:
    written += Print("%02X %02X %s %d", GetByte(module, i), GetByte(module, i+1), OpStr(op), GetByte(module, i + 1));
    break;
  default:
    written += Print("%02X    %s", GetByte(module, i), OpStr(op));
    break;
  }

  return written;
}

void Disassemble(char *title, StringMap *strings, Module *module)
{
  Print("───╴%s╶───\n", title);

  for (u32 i = 0; i < VecCount(module->code); i += OpSize(module->code[i])) {
    DisassembleInstruction(module, strings, i);
    Print("\n");
  }
  Print("\n");
}








