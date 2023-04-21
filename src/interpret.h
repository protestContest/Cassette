#pragma once
#include "parse.h"

// #define DEBUG_EVAL

Val RunFile(char *filename, Mem *mem);
Val Interpret(Val ast, Mem *mem);
Val RuntimeError(char *message, Val exp, Mem *mem);
Val Eval(Val exp, Val env, Mem *mem);
Val Apply(Val proc, Val args, Mem *mem);
