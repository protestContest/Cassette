#pragma once
#include "parse.h"

Val Interpret(Val ast, Mem *mem);
Val InitialEnv(Mem *mem);
