#pragma once
#include "chunk.h"
#include "node.h"

/* A Module keeps track of a module's various components. */

typedef struct {
  u32 module;
  u32 alias;
} ModuleImport;

typedef struct Module {
  u32 name;
  u32 id;
  char *filename;
  char *source;
  ModuleImport *imports;
  u32 *exports;
  ASTNode *ast;
  Chunk *code;
} Module;

void InitModule(Module *module);
void DestroyModule(Module *module);
