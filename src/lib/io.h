#pragma once
#include "vm.h"

Val IOPrint(VM *vm, Val args);
Val IOInspect(VM *vm, Val args);
Val IOOpen(VM *vm, Val args);
Val IORead(VM *vm, Val args);
Val IOWrite(VM *vm, Val args);
Val IOReadFile(VM *vm, Val args);
Val IOWriteFile(VM *vm, Val args);
