#pragma once

#ifdef __APPLE__
#include <Carbon/Carbon.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#else

typedef void** Handle;

Handle NewHandle(u32 size);
void DisposeHandle(Handle h);
u32 GetHandleSize(Handle h);
void SetHandleSize(Handle h, u32 size);

#endif
