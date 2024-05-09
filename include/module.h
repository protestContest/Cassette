#pragma once

typedef struct {
  i32 mod;
  i32 func;
} Import;

typedef struct {
  i32 name;
  i32 *exports;
  Import *imports;
  i32 *constants;
  char *data;
  u8 *code;
} Module;

void PushByte(u8 byte, Module *m);
void PushInt(i32 num, Module *m);
i32 ReadInt(i32 *index, Module *m);
void PushConst(i32 c, Module *m);
void PushData(char *data, i32 length, Module *m);
void InitModuleSymbols(Module *m);
