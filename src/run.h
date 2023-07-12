#pragma once
#include "mem.h"
#include "vm.h"

void RunFile(char *filename);
Val Eval(char *source, VM *vm);
void REPL(void);
