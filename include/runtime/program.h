#pragma once
#include "runtime/source_map.h"

/* A Program is run by the VM. */

typedef struct {
  u8 *code; /* vec */
  char *strings; /* vec */
  SourceMap srcmap;
  bool trace;
} Program;

Program *NewProgram(void);
void FreeProgram(Program *program);
u32 SerializeProgram(Program *program, u8 **dst);

#ifdef DEBUG
void DisassembleProgram(Program *program);
#endif
