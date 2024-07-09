#pragma once
#include "mem.h"

bool Define(val var, val value, val env);
val Lookup(val var, val env);
i32 FindEnv(val var, val env);

bool EnvSet(val value, u32 index, val env);
val EnvGet(u32 index, val env);
