#pragma once
#include "../value.h"
#include "../runtime/ops.h"

typedef struct {
  u8 *code;
  Val *constants;
  Val env;
} Module;

Module *CreateModule(void);

u32 ChunkSize(Module *module);
u32 PutByte(Module *module, u8 byte);
void SetByte(Module *module, u32 i, u8 byte);
u8 GetByte(Module *module, u32 i);
u8 PutConst(Module *module, Val value);
Val GetConst(Module *module, u32 i);
u32 PutInst(Module *module, OpCode op, ...);

typedef struct StringMap StringMap;

u32 DisassembleInstruction(Module *module, StringMap *strings, u32 i);
void Disassemble(char *title, StringMap *strings, Module *module);
