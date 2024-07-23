#pragma once

/* The parser produces an ASTNode from some input text. */

#include "result.h"
#include "module.h"

Result ParseModule(Module *module);
Result ParseModuleHeader(Module *module);
