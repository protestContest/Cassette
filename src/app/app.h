#pragma once

/* Set FONT_PATH and DEFAULT_FONT_PATH in Makefile */

#ifndef FONT_PATH
#define DEFAULT_FONT_PATH 0
#else
#define DEFAULT_FONT_PATH FONT_PATH
#endif

#ifndef DEFAULT_FONT
#define DEFAULT_FONT 0
#endif

typedef bool (*UpdateFn)(void *arg);

void InitApp(void);
char *FontPath(void);
void MainLoop(UpdateFn update, void *arg);
