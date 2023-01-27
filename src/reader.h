#pragma once
#include "value.h"

typedef struct {
  Status status;
  char *file;
  u32 line;
  u32 col;
  char *src;
  u32 cur;
  u32 indent;
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
char Advance(Reader *r);
void AdvanceLine(Reader *r);
void Retreat(Reader *r, u32 count);

void PrintSource(Reader *r);
void PrintSourceContext(Reader *r, u32 num_lines);
void PrintReaderError(Reader *r);

bool IsSymChar(char c);
bool IsOperator(char c);
void SkipSpace(Reader *r);
void SkipSpaceAndNewlines(Reader *r);
bool Check(Reader *r, const char *expect);
bool CheckToken(Reader *r, const char *expect);
bool Match(Reader *r, const char *expect);
void Expect(Reader *r, const char *expect);
