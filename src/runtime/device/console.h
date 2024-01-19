#pragma once
#include "runtime/mem/mem.h"
#include "univ/result.h"

Result ConsoleRead(void *context, Val length, Mem *mem);
Result ConsoleWrite(void *context, Val data, Mem *mem);
