#pragma once
#include "value.h"

Val Apply(Val proc, Val args);
Val EvalIn(Val exp, Val env);
Val Eval(Val exp);
