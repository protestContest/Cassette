#pragma once

/* The parser produces an ASTNode from some input text. */

#include "module.h"
#include "result.h"

Result ParseModuleBody(Module *module);
Result ParseModuleHeader(Module *module);
