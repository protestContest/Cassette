#pragma once
#include "vm.h"

Val Eval(VM *vm, Val exp, Val env);
Val Apply(VM *vm, Val proc, Val args);
