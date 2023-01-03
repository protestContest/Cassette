#include "reader.h"
#include "mem.h"
#include "printer.h"

typedef struct {
  char *src;
  u32 cur;
} Reader;

#define IsSpace(c)        ((c) == ' ' || (c) == '\n' || (c) == '\r' || (c) == '\t')
#define IsAlpha(c)        (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' & (c) <= 'z'))
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsReserved(c)     (IsSpace(c) || (c) == '(' || (c) == ')' || (c) == '[' || (c) == ']' || (c) == '\'' || (c) == ';' || (c) == '"')
#define IsUTFChar(c)      (((c) & 0x80) == 0x80)
#define IsSymChar(c)      (((c) > 0x20 && !IsReserved(c)) || IsUTFChar(c))
#define IsSymStart(c)     (IsSymChar(c) && !IsDigit(c))
#define IsEnd(c)          ((c) == '\0')

Val CurLocation(Reader *r)
{
  return IntVal(r->cur);
}

Val MakeToken(Val exp, Val location)
{
  return MakePair(exp, location);
}

Val TokenExp(Val token)
{
  return Head(token);
}

Val TokenLocation(Val token)
{
  return Tail(token);
}

void SkipSpace(Reader *r);

void SkipComment(Reader *r)
{
  while (r->src[r->cur] != '\n' && !IsEnd(r->src[r->cur])) {
    r->cur++;
  }
  r->cur++;
  SkipSpace(r);
}

void SkipSpace(Reader *r)
{
  while (IsSpace(r->src[r->cur])) {
    r->cur++;
  }
  if (r->src[r->cur] == ';') {
    SkipComment(r);
  }
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
  u32 start = r->cur;
  while (IsSymChar(r->src[r->cur])) {
    r->cur++;
  }
  Val symbol = MakeSymbol(&r->src[start], r->cur - start);
  return symbol;
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

Val ParseBinary(Reader *r)
{
  u32 start = r->cur;

  u32 size = 0;
  while (r->src[r->cur] != '"') {
    if (r->src[r->cur] == '\\') {
      r->cur++;
    }
    r->cur++;
    size++;
  }

  char str[size];
  u32 i = 0;
  r->cur = start;
  while (r->src[r->cur] != '"') {
    if (r->src[r->cur] == '\\') {
      r->cur++;
    }
    str[i] = r->src[r->cur];
    r->cur++;
    i++;
  }
  r->cur++;

  return MakeBinary(str, size);
}

Val ParseExpr(Reader *r)
{
  if (r->src[r->cur] == '\'') {
    r->cur++;
    SkipSpace(r);
    Val head = MakeSymbol("quote", 5);
    Val tail = ParseExpr(r);
    return MakePair(head, tail);
  } else if (r->src[r->cur] == '(') {
    r->cur++;
    SkipSpace(r);
    if (r->src[r->cur] == ')') {
      r->cur++;
      return nil;
    } else {
      return ParsePair(r);
    }
  } else if (r->src[r->cur] == '[') {
    r->cur++;
    SkipSpace(r);
    return ParseTuple(r);
  } else if (r->src[r->cur] == '"') {
    r->cur++;
    return ParseBinary(r);
  } else if (IsDigit(r->src[r->cur])) {
    return ParseInt(r);
  } else if (IsSymStart(r->src[r->cur])) {
    return ParseSym(r);
  } else {
    Error("Unexpected char 0x%02X at %d", (u8)r->src[r->cur], r->cur);
  }
}

Val Read(char *src)
{
  Reader r = { src, 0 };

  Val exps = MakePair(MakeSymbol("do", 2), nil);
  SkipSpace(&r);

  while (!IsEnd(r.src[r.cur])) {
    exps = MakePair(ParseExpr(&r), exps);
    SkipSpace(&r);
  }

  return Reverse(exps);
}

Val ReadFile(char *path)
{
  FILE *file = fopen(path, "r");
  if (!file) {
    Error("Could not open file \"%s\"", path);
  }

  fseek(file, 0, SEEK_END);
  u32 size = ftell(file);
  rewind(file);

  char src[size+1];
  for (u32 i = 0; i < size; i++) {
    int c = fgetc(file);
    src[i] = (char)c;
  }
  src[size] = '\0';

  return Read(src);
}
