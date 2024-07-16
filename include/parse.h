#pragma once

/*
The parser produces an AST from some input text.
*/

#include "node.h"

Node Parse(char *text);
