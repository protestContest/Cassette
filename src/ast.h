#pragma once
#include "parse.h"
#include "value.h"

Val AbstractNode(Parser *p, u32 sym, Val children);
void PrintAST(Val ast, Mem *mem);
