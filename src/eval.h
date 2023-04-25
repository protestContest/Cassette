#pragma once
#include "parse.h"
#include "vm.h"

// #define DEBUG_EVAL

Val RunFile(char *filename, Mem *mem);
Val Interpret(Val ast, VM *vm);
Val RuntimeError(char *message, Val exp, VM *vm);
Val Eval(Val exp, VM *vm);
Val Apply(Val proc, Val args, VM *vm);
