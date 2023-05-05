#pragma once
#include "parse.h"
#include "value.h"

Val ParseNode(u32 sym, Val children, Mem *mem);
