#pragma once
#include "heap.h"

Val OkResult(Val value, Heap *mem);
Val ErrorResult(char *error, Heap *mem);
