#pragma once
#include "parse.h"

Val Interpret(Val ast, Mem *mem);
Val RuntimeError(char *message, Val exp, Mem *mem);
