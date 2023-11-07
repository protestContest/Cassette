#pragma once

#include "result.h"
#include "runtime/chunk.h"

Result BuildProject(char *manifest, Chunk *chunk);
Result BuildScripts(u32 num_files, char **filenames, Chunk *chunk);
