#pragma once

#include "mem/mem.h"

typedef struct {
  char *message;
  char *filename;
  u32 pos;
  Val value;
  void *item;
} ErrorDetails;

typedef struct {
  bool ok;
  union {
    Val value;
    void *item;
    ErrorDetails *error;
  } data;
} Result;

#define ResultValue(r)  (r).data.value
#define ResultItem(r)   (r).data.item
#define ResultError(r)  (r).data.error

Result ValueResult(Val value);
Result ItemResult(void *item);
Result ErrorResult(char *message, char *filename, u32 pos);
Result OkError(Result error, Mem *mem);
