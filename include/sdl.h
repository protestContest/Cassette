#pragma once
#include "result.h"
#include "vm.h"

Result SDLNewWindow(VM *vm);
Result SDLDestroyWindow(VM *vm);
Result SDLPresent(VM *vm);
Result SDLClear(VM *vm);
Result SDLLine(VM *vm);
Result SDLSetColor(VM *vm);
Result SDLGetColor(VM *vm);
Result SDLPollEvent(VM *vm);
Result SDLGetTicks(VM *vm);
Result SDLBlit(VM *vm);
