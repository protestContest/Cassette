#pragma once
#include "result.h"

Result SystemSet(void *context, Val key, Val value, Mem *mem);
Result SystemGet(void *context, Val key, Mem *mem);
