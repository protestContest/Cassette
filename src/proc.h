#pragma once
#include "value.h"

Val MakeProc(Val params, Val body, Val env);
Val ProcParams(Val proc);
Val ProcBody(Val proc);
Val ProcEnv(Val proc);

void DefinePrimitive(char *name, Val env);
Val DoPrimitive(Val name, Val args);
