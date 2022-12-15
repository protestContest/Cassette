#pragma once

#include "mem.h"

Value Read(void);
Value Parse(char *src);
void DumpAST(Value ast);
