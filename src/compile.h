#pragma once
#include "value.h"
#include "vm.h"
#include "inst.h"

InstSeq Compile(Val exp, Reg target, Val linkage);
