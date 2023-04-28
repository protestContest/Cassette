#pragma once
#include "parse.h"
#include "value.h"

Val MakeTerm(Val value, Val line, Val col, Mem *mem);
bool IsTerm(Val node, Mem *mem);
Val TermVal(Val node, Mem *mem);
Val TermLine(Val node, Mem *mem);
Val TermCol(Val node, Mem *mem);
void PrintTerm(Val node, Mem *mem);
void PrintTerms(Val children, Mem *mem);

Val ParseNode(u32 sym, Val children, Mem *mem);
void PrintAST(Val ast, Mem *mem);
