#pragma once
#include "value.h"

Val InitialEnv(void);
Val ExtendEnv(Val vars, Val vals, Val env);
Val Lookup(Val var, Val env);
void SetVariable(Val var, Val val, Val env);
void Define(Val var, Val val, Val env);
