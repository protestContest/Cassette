#pragma once
#include "mem.h"
#include "parse.h"
#include <univ/symbol.h>

val InferTypes(val node);
void PrintType(val type);
