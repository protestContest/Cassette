#pragma once
#include "value.h"

typedef struct {
  char *src;
  char *last_ok;
  enum {
    PARSE_OK,
    PARSE_INCOMPLETE,
    PARSE_ERROR
  } status;
  u32 cur;
  char *file;
  u32 line;
  u32 col;
  char *error;
  Val ast;
} Reader;

#define ReadOk(r)     ((r)->status == PARSE_OK)

#define ParseError(r, fmt, ...)               \
  do {                                        \
    (r)->status = PARSE_ERROR;                \
    PrintInto((r)->error, fmt, __VA_ARGS__);  \
  } while (0)

Reader *NewReader(void);
void FreeReader(Reader *r);

void PrintReaderSource(Reader *r, u32 before, u32 after);
void PrintReaderError(Reader *r);

void Read(Reader *r, char *src);
void ReadLine(Reader *r);
void ReadFile(Reader *r, char *path);
