#pragma once
#include "value.h"
#include "eval.h"

Val InitialEnv(void);
Val ExtendEnv(Val env, Val keys, Val vals);
Val AddFrame(Val env, u32 size);
void Define(Val name, Val value, Val env);
void DefineModule(Val name, Val defs, Val env);

Val GlobalEnv(Val env);
EvalResult Lookup(Val var, Val env);

void DumpEnv(Val env, bool all);
