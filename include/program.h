#pragma once
#include "source_map.h"

/* A Program is run by the VM. */

typedef struct {
  u8 *code;
  char *strings;
  SourceMap srcmap;
  bool trace;
} Program;

Program *NewProgram(void);
void FreeProgram(Program *program);
u32 SerializeProgram(Program *program, u8 **dst);
void DisassembleProgram(Program *program);
