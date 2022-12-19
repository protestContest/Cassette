#pragma once
#include "value.h"

#define Line(loc)               Head(loc)
#define Col(loc)                Tail(loc)

#define Text(source)            Head(source)
#define Cursor(source)          Tail(source)

Value Read(void);
Value Parse(Value source, Value ast);
void DumpAST(Value ast);
