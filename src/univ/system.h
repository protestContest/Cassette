#pragma once

#define Assert(expr) ((expr) || (Alert("Assert failed in " __FILE__ ": " #expr), Exit(), 0))
typedef void **Handle;

void Copy(void *src, void *dst, u32 size);
void Exit(void);
void Alert(char *message);
int Open(char *path);
char *ReadFile(char *path);
u32 Ticks(void);
