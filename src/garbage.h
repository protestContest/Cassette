#pragma once
#include "mem.h"
#include "vm.h"

void CollectGarbage(VM *vm);
void MaybeCollectGarbage(VM *vm);
