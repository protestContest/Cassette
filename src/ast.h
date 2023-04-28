#pragma once
#include "parse.h"
#include "value.h"

Val MakeNode(Val value, Val line, Val col, Mem *mem);
bool IsNode(Val node, Mem *mem);
Val NodeVal(Val node, Mem *mem);
Val NodeLine(Val node, Mem *mem);
Val NodeCol(Val node, Mem *mem);
void PrintNode(Val node, Mem *mem);
void PrintNodes(Val children, Mem *mem);

Val ParseNode(u32 sym, Val children, Mem *mem);
void PrintAST(Val ast, Mem *mem);
