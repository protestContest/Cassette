#pragma once
#include "value.h"

Val Eval(Val exp, Val env);
Val Apply(Val proc, Val args);
