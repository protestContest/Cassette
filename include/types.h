#pragma once

/*
The type checker infers types using the Hindley-Milner type system.
*/

#include "mem.h"
#include "node.h"

typedef val Type;

Node InferTypes(Node node);
void PrintType(Type type);
Type ParseType(char *spec);
val TypeStr(Type type);
