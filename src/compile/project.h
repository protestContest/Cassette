#pragma once
#include "result.h"
#include "runtime/chunk.h"

Result BuildProject(u32 num_files, char **filenames, char *stdlib, Chunk *chunk);
