#pragma once
#include "value.h"

void DebugVal(Val val);
u32 ValStrLen(Val val);
char *ValStr(Val val);
char *ValAbbr(Val val);
void PrintVal(Val val);
void PrintTree(Val exp);
