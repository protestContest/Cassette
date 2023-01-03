#pragma once
#include "value.h"

Val Read(char *src);
Val ReadFile(char *path);

Val TokenExp(Val token);
Val TokenLocation(Val token);
