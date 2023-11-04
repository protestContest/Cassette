#pragma once

#include "result.h"
#include "runtime/chunk.h"

BuildResult BuildProject(char *manifest, Chunk *chunk);
