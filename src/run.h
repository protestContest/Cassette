#pragma once
#include "args.h"
#include "mem.h"
#include "vm.h"

#define VERSION "0.0.1"

void RunFile(Opts opts);
void CompileFile(char *filename, char *module_path);
Val Eval(Val expr, VM *vm);
void REPL(Opts opts);

void PrintVersion(void);
