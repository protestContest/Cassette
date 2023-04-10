#pragma once
#include "../compile/parse.h"

Val Interpret(ASTNode *node, Val *mem);
