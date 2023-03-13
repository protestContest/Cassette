// #include "module.h"
// #include <univ/vec.h>
// #include <univ/allocate.h>
// #include <univ/io.h>
// #include <univ/str.h>
// #include "../runtime/print.h"
// #include <stdarg.h>

// Module *CreateModule(void)
// {
//   Module *module = Allocate(sizeof(Module));
//   module->code = NULL;
//   module->constants = NULL;
//   module->env = nil;
//   return module;
// }

// u32 ChunkSize(Module *module)
// {
//   return VecCount(module->code);
// }

// u32 PutByte(Module *module, u8 byte)
// {
//   VecPush(module->code, byte);
//   return VecCount(module->code) - 1;
// }

// void SetByte(Module *module, u32 i, u8 byte)
// {
//   module->code[i] = byte;
// }

// u8 GetByte(Module *module, u32 i)
// {
//   return module->code[i];
// }

// u8 PutConst(Module *module, Val value)
// {
//   for (u32 i = 0; i < VecCount(module->constants); i++) {
//     if (Eq(module->constants[i], value)) return i;
//   }

//   VecPush(module->constants, value);
//   return VecCount(module->constants) - 1;
// }

// Val GetConst(Module *module, u32 i)
// {
//   return module->constants[i];
// }

// u32 PutInst(Module *module, OpCode op, ...)
// {
//   va_list args;
//   va_start(args, op);

//   PutByte(module, op);

//   if (op == OP_CONST) {
//     PutByte(module, PutConst(module, va_arg(args, Val)));
//   } else if (OpSize(op) > 1) {
//     PutByte(module, va_arg(args, u32));
//   }

//   va_end(args);
//   return VecCount(module->code) - OpSize(op);
// }
