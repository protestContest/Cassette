#pragma once
#include "chunk.h"
#include "node.h"
#include "univ/hashmap.h"

/* A Module keeps track of a module's various components. */

typedef struct Module {
  u32 id;
  char *filename;
  char *source;
  ASTNode *ast;
  Chunk *code;
  HashMap exports;
} Module;
#define ModuleName(mod) NodeChild((mod)->ast, 0)
#define ModuleImports(mod) NodeChild((mod)->ast, 1)
#define ModuleExports(mod) NodeChild((mod)->ast, 2)
#define ModuleBody(mod) NodeChild((mod)->ast, 3)

void InitModule(Module *module);
void DestroyModule(Module *module);
