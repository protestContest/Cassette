#include "module.h"

void InitModule(Module *module)
{
  module->id = 0;
  module->filename = 0;
  module->source = 0;
  module->ast = 0;
  module->code = 0;
  InitHashMap(&module->exports);
}

void DestroyModule(Module *module)
{
  if (module->filename) free(module->filename);
  if (module->source) free(module->source);
  if (module->ast) FreeNode(module->ast);
  if (module->code) FreeChunk(module->code);
  DestroyHashMap(&module->exports);
  module->id = 0;
  module->filename = 0;
  module->source = 0;
  module->ast = 0;
  module->code = 0;
}
