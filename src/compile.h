#pragma once
#include "mem.h"
#include "vm.h"

// #define DEBUG_COMPILE

Val Compile(Val exp, Mem *mem);
void PrintSeq(Val stmts, Mem *mem);
