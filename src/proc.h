#pragma once
#include "value.h"

Val MakeProc(Val name, Val params, Val body, Val env);
bool IsProc(Val proc);
Val ProcName(Val proc);
Val ProcParams(Val proc);
Val ProcBody(Val proc);
Val ProcEnv(Val proc);
