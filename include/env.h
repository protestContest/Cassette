#pragma once
#include "mem.h"

val ExtendEnv(u32 frame_size, val env);

bool Define(val var, val value, val env);
val Lookup(val var, val env);
i32 FindEnv(val var, val env);
i32 FindFrame(val var, val env);

bool EnvSet(val value, u32 index, val env);
val EnvGet(u32 index, val env);

void PrintEnv(val env);
