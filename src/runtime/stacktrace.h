#pragma once
#include "vm.h"

typedef struct StackTraceItem {
  char *filename;
  u32 position;
  struct StackTraceItem *prev;
} StackTraceItem;

StackTraceItem *NewStackTraceItem(char *filename, u32 position, StackTraceItem *prev);
void FreeStackTrace(StackTraceItem *trace);
StackTraceItem *StackTrace(VM *vm);
