#pragma once
#include "node.h"
#include "source_map.h"
#include "univ/hashmap.h"
#include "univ/vec.h"

/* A Program is run by the VM. */

typedef struct {
  VecOf(u8) **code;
  VecOf(char) **strings;
  SourceMap srcmap;
  bool trace;
} Program;

Program *NewProgram(void);
void FreeProgram(Program *program);
void AddStrings(ASTNode *node, Program *program, HashMap *strings);
u32 SerializeProgram(Program *program, u8 **dst);

#ifdef DEBUG
void DisassembleProgram(Program *program);
#endif
