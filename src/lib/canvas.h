#pragma once
#include "vm.h"

#ifdef WITH_CANVAS
bool InitCanvas(u32 width, u32 height);
void DestroyCanvas(void);
bool ShouldShowCanvas(void);
void ShowCanvas(void);
Val CanvasLine(VM *vm, Val args);
Val CanvasWidth(VM *vm, Val args);
Val CanvasHeight(VM *vm, Val args);
#endif
