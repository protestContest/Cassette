#pragma once

#include "mem.h"

Val MakeBignum(i32 num, Mem *mem);
Val AddBignum(Val a, Val b, Mem *mem);
