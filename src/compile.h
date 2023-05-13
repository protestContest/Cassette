#pragma once
#include "mem.h"
#include "vm.h"

Val Compile(Val exp, Mem *mem);
void PrintStmts(Val stmts, Mem *mem);
