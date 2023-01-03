#pragma once
#include "value.h"

Val MakeProc(Val name, Val params, Val body, Val env);
Val ProcName(Val proc);
Val ProcParams(Val proc);
Val ProcBody(Val proc);
Val ProcEnv(Val proc);
