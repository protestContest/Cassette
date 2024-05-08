#pragma once

typedef struct {
  i32 *constants;
  char *data;
  u8 *code;
} Program;

void PushByte(u8 byte, Program *p);
void PushInt(i32 num, Program *p);
i32 ReadInt(i32 *index, Program *p);
void PushConst(i32 c, Program *p);
void PushData(char *data, i32 length, Program *p);
void InitProgramSymbols(Program *p);
