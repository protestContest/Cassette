#pragma once
#include "value.h"
#include "scan.h"
#include "chunk.h"

Status Compile(char *src, Chunk *chunk);
