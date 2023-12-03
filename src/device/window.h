#pragma once
#include "result.h"

Result WindowOpen(Val opts, Mem *mem);
Result WindowClose(void *context, Mem *mem);
Result WindowRead(void *context, Val length, Mem *mem);
Result WindowWrite(void *context, Val data, Mem *mem);
Result WindowSet(void *context, Val key, Val value, Mem *mem);
Result WindowGet(void *context, Val key, Mem *mem);
