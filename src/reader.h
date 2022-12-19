#pragma once
#include "value.h"

#define LineNum(cur)            Head(cur)
#define Column(cur)             Tail(cur)

#define SourceLines(source)     Head(source)
#define SourcePos(source)       Tail(source)
#define CurrentLine(source)     Head(SourceLines(source))
#define CurrentLineNum(source)  LineNum(SourcePos(source))
#define CurrentColumn(source)   Column(SourcePos(source))

Value Read(void);
Value Parse(Value source, Value ast);
void DumpAST(Value ast);
