#pragma once

/*
An error object stores an unformatted message object and the source position of
the error.
*/

#include "mem.h"

typedef val Error;

#define IsError(err)          (IsTuple(err) && TupleGet(err, 0) == SymVal(Symbol("error")))
#define ErrorMsg(err)         TupleGet(err, 1)
#define ErrorStart(err)       Head(TupleGet(err, 2))
#define ErrorEnd(err)         Tail(TupleGet(err, 2))

Error MakeError(val msg, val position);
val PrefixError(Error error, char *prefix);
void PrintError(Error error, char *source);
