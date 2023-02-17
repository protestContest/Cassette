#pragma once
#include "../buf.h"
#include <unistd.h>

#ifdef _POSIX_VERSION
#define OutputBuf(data, cap)  IOBuf(1, data, cap)
#else
#define OutputBuf(data, cap)  MemBuf(data, cap)
#endif

bool OSWrite(int file, void *data, u32 length);
