#pragma once
#include "value.h"
#include "vm.h"
#include "chunk.h"

Chunk *Compile(Val exp, Reg target, Val linkage);
Chunk *CompileLinkage(Val linkage);
Chunk *EndWithLinkage(Val linkage, Chunk *seq);
