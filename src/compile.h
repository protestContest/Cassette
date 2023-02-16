#pragma once
#include "value.h"
#include "scan.h"
#include "chunk.h"

#define DEBUG_COMPILE 1

Status Compile(char *src, Chunk *chunk);
Status CompileModule(char *src, Chunk *chunk);
