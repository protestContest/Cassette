#pragma once
#include "value.h"

Val InitialEnv(void);
Val BaseEnv(Val env);
Val ExtendEnv(Val vars, Val vals, Val env);
Val Lookup(Val var, Val env);
void SetVariable(Val var, Val val, Val env);
void Define(Val var, Val val, Val env);

bool IsEnv(Val env);

Val FirstFrame(Val env);
Val FrameVars(Val frame);
Val FrameVals(Val frame);

void DumpEnv(Val env);
