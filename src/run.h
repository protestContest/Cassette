#pragma once
#include "mem.h"
#include "vm.h"

#define VERSION "0.0.1"

void RunFile(char *filename);
void CompileFile(char *filename);
Val Eval(Val expr, VM *vm);
void REPL(void);

void PrintVersion(void);
