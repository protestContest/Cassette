#pragma once
#include "value.h"

typedef struct {
  enum {
    PARSE_OK,
    PARSE_INCOMPLETE,
    PARSE_ERROR
  } status;
  char *file;
  u32 line;
  u32 col;
  char *src;
  u32 cur;
  u32 last_ok;
  char *error;
  Val ast;
} Reader;

#define Peek(r)           ((r)->src[(r)->cur])

Reader *NewReader(void);
void FreeReader(Reader *r);

void Read(Reader *r, char *src);
void ReadFile(Reader *r, char *path);
void CancelRead(Reader *r);

void AppendSource(Reader *r, char *src);
Val Stop(Reader *r);
Val ParseError(Reader *r, char *msg);
void Rewind(Reader *r);
void Advance(Reader *r);
void AdvanceLine(Reader *r);
void Retreat(Reader *r);

void PrintSource(Reader *r);
void PrintSourceContext(Reader *r, u32 num_lines);
void PrintReaderError(Reader *r);
