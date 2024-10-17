#pragma once
#include "node.h"
#include "source_map.h"
#include "univ/hashmap.h"

/* A Program is run by the VM. */

typedef struct {
  u8 *code;
  char *strings;
  SourceMap srcmap;
  bool trace;
} Program;

Program *NewProgram(void);
void FreeProgram(Program *program);
void AddStrings(ASTNode *node, Program *program, HashMap *strings);
u32 SerializeProgram(Program *program, u8 **dst);
void DisassembleProgram(Program *program);
