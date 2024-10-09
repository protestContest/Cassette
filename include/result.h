#pragma once
#include "error.h"

/* A general-purpose result structure */

typedef struct {
  bool ok;
  union {
    void *p;
    u32 v;
  } data;
} Result;
#define IsError(result)    (!(result).ok)



Result Ok(void *data);
Result OkVal(u32 value);
Result Err(Error *error);
