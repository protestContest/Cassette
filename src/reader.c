#include "reader.h"
#include "mem.h"
#include "printer.h"

typedef struct {
  char *src;
  u32 start;
  u32 cur;
} Reader;

#define IsSpace(c)    ((c) == ' ' || (c) == '\n' || (c) == '\r' || (c) == '\t')
#define IsAlpha(c)    (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' & (c) <= 'z'))
#define IsDigit(c)    ((c) >= '0' && (c) <= '9')
#define IsReserved(c) (IsSpace(c) || (c) == '(' || (c) == ')' || (c) == '[' || (c) == ']' || (c) == '\'' || (c) == ';')
#define IsSymChar(c)  ((c) >= '!' && (c) <= '~' && !IsReserved(c))
#define IsSymStart(c) (IsSymChar(c) && !IsDigit(c))

void SkipSpace(Reader *r)
{
  while (IsSpace(r->src[r->cur])) {
    r->cur++;
  }
  r->start = r->cur;
}

Val ParseInt(Reader *r)
{
  u32 n = r->src[r->cur] - '0';
  r->cur++;
  while (IsDigit(r->src[r->cur])) {
    u32 d = r->src[r->cur] - '0';
    n = n*10 + d;
    r->cur++;
  }
  return IntVal(n);
}

Val ParseSym(Reader *r)
{
  while (IsSymChar(r->src[r->cur])) r->cur++;
  return MakeSymbol(&r->src[r->start], r->cur - r->start);
}

Val ParseExpr(Reader *r);

Val ParsePair(Reader *r)
{
  Val head = ParseExpr(r);
  SkipSpace(r);

  if (r->src[r->cur] == '.') {
    r->cur++;
    SkipSpace(r);
    Val tail = ParseExpr(r);
    SkipSpace(r);
    if (r->src[r->cur] != ')') {
      Error("Malformed pair");
    }
    r->cur++;
    SkipSpace(r);
    return MakePair(head, tail);
  }

  if (r->src[r->cur] == ')') {
    r->cur++;
    SkipSpace(r);
    return MakePair(head, nil);
  }

  Val tail = ParsePair(r);
  return MakePair(head, tail);
}

Val ParseTuple(Reader *r)
{
  if (r->src[r->cur] == ']') {
    return MakeTuple(0);
  }

  Val items = nil;
  u32 count = 0;
  while (r->src[r->cur] != ']') {
    Val item = ParseExpr(r);
    items = MakePair(item, items);
    count++;
    SkipSpace(r);
  }

  Val tuple = MakeTuple(count);
  for (u32 i = 0; i < count; i++) {
    TupleSet(tuple, count-1-i, Head(items));
    items = Tail(items);
  }

  r->cur++;
  SkipSpace(r);
  return tuple;
}

Val ParseExpr(Reader *r)
{
  if (r->src[r->cur] == '\'') {
    r->cur++;
    SkipSpace(r);
    Val head = MakeSymbol("quote", 5);
    Val tail = ParseExpr(r);
    return MakePair(head, tail);
  }

  if (r->src[r->cur] == '(') {
    r->cur++;
    SkipSpace(r);
    if (r->src[r->cur] == ')') {
      r->cur++;
      return nil;
    } else {
      return ParsePair(r);
    }
  }

  if (r->src[r->cur] == '[') {
    r->cur++;
    SkipSpace(r);
    return ParseTuple(r);
  }

  if (IsDigit(r->src[r->cur])) {
    return ParseInt(r);
  }

  if (IsSymStart(r->src[r->cur])) {
    return ParseSym(r);
  }

  if (r->src[r->cur] == ';') {
    while (r->src[r->cur] != '\n') {
      r->cur++;
    }
    r->cur++;
    SkipSpace(r);
    return ParseExpr(r);
  }

  Error("Unexpected char at %d", r->cur);
}

Val Read(char *src)
{
  Reader r = { src, 0, 0 };
  SkipSpace(&r);
  return ParseExpr(&r);
}
