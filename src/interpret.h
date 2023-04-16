#pragma once
#include "parse.h"

Val Interpret(Val ast, Mem *mem);
Val InitialEnv(Mem *mem);
Val RuntimeError(char *message, Val exp, Mem *mem);
bool IsTrue(Val value);
void Define(Val env, Val var, Val value, Mem *mem);
