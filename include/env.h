#pragma once

/*
An environment is a linked list of frames. Each frame is a tuple containing
values in the environment. At compile time, the environment stores variable
names to determine their location. At runtime, the environment stores values
which can be looked up by their location.
*/

#include "mem.h"

typedef val Env;

Env ExtendEnv(u32 frame_size, Env env);

bool Define(val var, val value, Env env);
val Lookup(val var, Env env);
i32 FindEnv(val var, Env env);
i32 FindFrame(val var, Env env);

bool EnvSet(val value, u32 index, Env env);
val EnvGet(u32 index, Env env);

void PrintEnv(Env env);
