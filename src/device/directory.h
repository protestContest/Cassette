#pragma once
#include "result.h"

Result DirectoryOpen(Val opts, Mem *mem);
Result DirectoryClose(void *context, Mem *mem);
Result DirectoryRead(void *context, Val length, Mem *mem);
Result DirectoryWrite(void *context, Val data, Mem *mem);
Result DirectoryGet(void *context, Val key, Mem *mem);
