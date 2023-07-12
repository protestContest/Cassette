#pragma once
#include "mem.h"
#include "vm.h"

void RunFile(char *filename);
Val Eval(Val expr, VM *vm);
void REPL(void);
