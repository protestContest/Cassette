#pragma once

#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define Assert(expr)          assert(expr)
#define Alloc(size)           malloc(size)
#define Realloc(ptr, size)    realloc(ptr, size)
#define Free(ptr)             free(ptr)
#define Copy(from, to, size)  memcpy(to, from, size)
#define GetEnv(name)          getenv(name)
#define Time()                time(0)
#define Exit()                exit(0)
