#pragma once

typedef struct {
  char *message;
  char *filename;
  u32 pos;
  u32 value;
  void *item;
} ErrorDetails;

typedef struct {
  bool ok;
  union {
    u32 value;
    void *item;
    ErrorDetails *error;
  } data;
} Result;

#define ResultValue(r)  (r).data.value
#define ResultItem(r)   (r).data.item
#define ResultError(r)  (r).data.error

Result ValueResult(u32 value);
Result ItemResult(void *item);
Result ErrorResult(char *message, char *filename, u32 pos);
