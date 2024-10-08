#pragma once

/* The parser produces an ASTNode from some input text. */

#include "node.h"

ASTNode *ParseModule(char *source);
