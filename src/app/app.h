#pragma once
#include "canvas.h"

typedef bool (*UpdateFn)(void *arg);

void InitApp(void);
void MainLoop(UpdateFn update, void *arg);
char *FontPath(char *name);
void AddCanvas(Canvas *canvas);
