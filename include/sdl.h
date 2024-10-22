#pragma once
#ifdef SDL
#include "vm.h"

u32 SDLNewWindow(VM *vm);
u32 SDLDestroyWindow(VM *vm);
u32 SDLPresent(VM *vm);
u32 SDLClear(VM *vm);
u32 SDLLine(VM *vm);
u32 SDLSetColor(VM *vm);
u32 SDLGetColor(VM *vm);
u32 SDLPollEvent(VM *vm);
u32 SDLGetTicks(VM *vm);
u32 SDLBlit(VM *vm);
#endif
