#pragma once
#include "mem.h"
#include "vm.h"

Val Compile(Val exp, Mem *mem);
void PrintSeq(Val stmts, Mem *mem);
