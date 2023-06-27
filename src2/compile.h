#pragma once
#include "chunk.h"

Chunk Compile(char *source);
void Disassemble(Chunk *chunk);
