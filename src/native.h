#pragma once
#include "value.h"
#include "native_map.h"

typedef struct VM VM;

void DefineNatives(VM *vm);
void DoNative(VM *vm, Val name, u32 num_args);
