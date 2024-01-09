#pragma once

typedef bool (*UpdateFn)(void *arg);

void InitApp(void);
void MainLoop(UpdateFn update, void *arg);
char *FontPath(char *name);
