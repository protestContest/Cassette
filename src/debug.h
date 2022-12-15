#pragma once
#include "value.h"

void DebugTable(char *title, u32 width, u32 num_cols);
void DebugValue(Value value, u32 len);

#define UNDERLINE_START "\x1B[4m"
#define UNDERLINE_END   "\x1B[0m"

#define DebugEmpty()    printf("\n    (empty)\n\n")
#define DebugRow()      printf("\n  ")
#define DebugCol()      printf(" │ ")
#define EndDebugTable() printf("\n\n")
