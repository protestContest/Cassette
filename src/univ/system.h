#pragma once

#define Assert(expr) ((expr) || (Alert("Assert failed: " #expr), Exit(), 0))
typedef void **Handle;

Handle NewHandle(u32 size);
void SetHandleSize(Handle handle, u32 size);
void DisposeHandle(Handle handle);
void Copy(void *src, void *dst, u32 size);
void Exit(void);
void Alert(char *message);
