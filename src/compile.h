#pragma once
#include "chunk.h"
#include "lex.h"
#include "assemble.h"
#include "module.h"

Seq Compile(Val ast, Mem *mem);
