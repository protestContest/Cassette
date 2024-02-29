#pragma once
#include "ast.h"
#include "univ/result.h"

Result CompileFunc(Node *lambda);
Result CompileModule(Node *ast);
