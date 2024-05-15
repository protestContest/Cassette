#pragma once
#include "mem.h"

typedef struct {
  i32 *constants;
  char *data;
  u8 *code;
  u32 *srcMap;
} Module;

void InitModule(Module *mod);
void DestroyModule(Module *mod);
void PushByte(u8 byte, u32 pos, Module *mod);
i32 IntSize(i32 num);
void PushInt(i32 num, u32 pos, Module *mod);
i32 ReadInt(u32 *index, Module *mod);
void PushLabel(u32 index, u32 pos, Module *mod);
void AppendCode(u8 *code, u32 *srcMap, Module *mod);
void PushConst(val c, u32 pos, Module *mod);
void PushData(char *data, i32 length, Module *mod);
char *DisassembleOp(u32 *index, char *text, Module *mod);
char *Disassemble(Module *mod);
u32 GetSourcePos(u32 index, Module *mod);
