#pragma once
#include "value.h"
#include "mem.h"
#include "lex.h"

Val Parse(char *src, Mem *mem);
void PrintAST(Val ast, Mem *mem);
