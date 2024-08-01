#pragma once
#include "result.h"
#include "vm.h"

Result SDLNewWindow(VM *vm);
Result SDLPresent(VM *vm);
Result SDLClear(VM *vm);
Result SDLLine(VM *vm);
Result SDLSetColor(VM *vm);
Result SDLPollEvent(VM *vm);
