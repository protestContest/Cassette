#pragma once
#include "runtime/source_map.h"
#include "univ/iff.h"
#include "runtime/error.h"

/*
A Program is run by the VM.

`code` is the bytecode instructions.
`strings` is a sequence of null-terminated symbols and strings that appear in
the program.
`srcmap` maps bytecode addresses to source file positions.

A program can be serialized into a "tape" file, which contains the bytecode and
strings. The file format is an IFF form of type 'TAPE'; it has two fields:

- 'CODE': Bytecode
- 'STRS': Strings
- 'FMAP': Source file map (optional, unimplemented)
- 'SMAP': Source position map (optional, unimplemented)

Each field is compressed using GIF-flavored LZW, with 8-bit symbols.
*/

typedef struct {
  u8 *code; /* vec */
  char *strings; /* vec */
  SourceMap srcmap;
} Program;

Program *NewProgram(void);
void FreeProgram(Program *program);
IFFChunk *SerializeProgram(Program *program);
Error *DeserializeProgram(IFFChunk *chunk, Program **program);
Error *ReadProgramFile(char *filename, Program **program);
void WriteProgramFile(Program *program, char *filename);

#ifdef DEBUG
void DisassembleProgram(Program *program);
#endif
