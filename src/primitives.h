#pragma once
#include "value.h"

void DefinePrimitives(Val env);

Val DoPrimitive(Val name, Val args);
bool IsTrue(Val val);
