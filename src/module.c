#include "module.h"
#include "parse.h"
#include "univ/vec.h"

void InitModule(Module *module)
{
  module->name = 0;
  module->id = 0;
  module->filename = 0;
  module->source = 0;
  module->imports = 0;
  module->exports = 0;
  module->ast = 0;
  module->code = 0;
}

void DestroyModule(Module *module)
{
  u32 i;
  for (i = 0; i < VecCount(module->imports); i++) {
    FreeVec(module->imports[i].names);
  }
  if (module->filename) free(module->filename);
  if (module->source) free(module->source);
  if (module->imports) FreeVec(module->imports);
  if (module->exports) FreeVec(module->exports);
  if (module->ast) FreeNode(module->ast);
  if (module->code) FreeChunk(module->code);
  InitModule(module);
}
