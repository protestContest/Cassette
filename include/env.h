#pragma once
#include "mem.h"

bool Define(val var, u32 index, val env);
i32 FindEnv(val var, val env);
val Lookup(u32 index, val env);
