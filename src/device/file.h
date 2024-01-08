#pragma once
#include "mem/mem.h"
#include "univ/result.h"

Result FileOpen(Val opts, Mem *mem);
Result FileClose(void *context, Mem *mem);
Result FileRead(void *context, Val length, Mem *mem);
Result FileWrite(void *context, Val data, Mem *mem);
Result FileSet(void *context, Val key, Val value, Mem *mem);
Result FileGet(void *context, Val key, Mem *mem);
