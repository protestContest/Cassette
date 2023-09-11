#pragma once

#define Assert(expr) ((expr) || (Exit(), 0))

void Exit(void);
i32 Open(char *path);
bool Read(i32 file, void *data, u32 length);
bool Write(i32 file, void *data, u32 length);

