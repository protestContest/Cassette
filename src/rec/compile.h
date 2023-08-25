#pragma once
#include "heap.h"
#include "seq.h"
#include "module.h"

ModuleResult Compile(Val ast, Val env, Heap *mem);
