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
i32 IntSize(i32 num);
i32 ReadInt(u32 *index, Module *mod);
char *DisassembleOp(u32 *index, char *text, Module *mod);
char *Disassemble(Module *mod);
u32 GetSourcePos(u32 index, Module *mod);
