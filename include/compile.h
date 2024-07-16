#pragma once

/* The compiler takes an AST node and compiles it into a chunk. */

#include "chunk.h"
#include "node.h"

Chunk Compile(Node node);
