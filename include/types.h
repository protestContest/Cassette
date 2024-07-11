#pragma once
#include "mem.h"
#include "parse.h"
#include <univ/symbol.h>

val InferTypes(val node, val context);
val Generalize(val type, val context);
void PrintType(val type);
val ParseType(char *spec);
