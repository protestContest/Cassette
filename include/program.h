#pragma once
#include "mem.h"

/* A Program is run by the VM. */

typedef struct {
  u8 *code;
  char *symbols;
} Program;

Program *NewProgram(void);
void FreeProgram(Program *program);
u32 SerializeProgram(Program *program, u8 **dst);
