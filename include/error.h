#pragma once
#include "mem.h"

#define IsError(err)          (IsTuple(err) && TupleGet(err, 0) == 0)
#define ErrorMsg(err)         TupleGet(err, 1)
#define ErrorStart(err)       Head(TupleGet(err, 2))
#define ErrorEnd(err)         Tail(TupleGet(err, 2))

val MakeError(val msg, val positions);
val PrefixError(val error, char *prefix);
void PrintError(val error, char *source);
