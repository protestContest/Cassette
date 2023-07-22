#pragma once
#include "mem.h"

Val ModuleName(Val ast, Mem *mem);
Val Parse(char *source, Mem *mem);
void PrintAST(Val ast, Mem *mem);
