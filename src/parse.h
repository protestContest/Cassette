#pragma once
#include "mem.h"

Val Parse(char *source, Mem *mem);
void PrintAST(Val ast, Mem *mem);
