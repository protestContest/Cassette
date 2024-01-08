#pragma once
#include "mem/mem.h"
#include "univ/result.h"

Result SerialOpen(Val opts, Mem *mem);
Result SerialClose(void *context, Mem *mem);
Result SerialRead(void *context, Val length, Mem *mem);
Result SerialWrite(void *context, Val data, Mem *mem);
Result SerialSet(void *context, Val key, Val value, Mem *mem);
Result SerialGet(void *context, Val key, Mem *mem);
