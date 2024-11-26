#pragma once
#include "runtime/source_map.h"

/*
A Program is run by the VM.

`code` is the bytecode instructions.
`strings` is a sequence of null-terminated symbols and strings that appear in
the program.
`srcmap` maps bytecode addresses to source file positions.
*/

typedef struct {
  u8 *code; /* vec */
  char *strings; /* vec */
  SourceMap srcmap;
} Program;

Program *NewProgram(void);
void FreeProgram(Program *program);
u32 SerializeProgram(Program *program, u8 **dst);

#ifdef DEBUG
void DisassembleProgram(Program *program);
#endif
