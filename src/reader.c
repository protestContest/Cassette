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
#define IsUTFChar(c)      (((c) & 0x80) == 0x80)
#define IsSymChar(c)      (((c) > 0x20 && !IsReserved(c)) || IsUTFChar(c))
#define IsSymStart(c)     (IsSymChar(c) && !IsDigit(c))
#define IsEnd(c)          ((c) == '\0')

bool IsReserved(char c)
{
  #define NUM_RESERVED 12
  static char reserved[NUM_RESERVED] = {'(', ')', '[', ']', '{', '}', ':', ';', '"', '.', '|'};

  if (IsSpace(c)) return true;
  for (u32 i = 0; i < NUM_RESERVED; i++) {
    if (c == reserved[i]) return true;
  }
  return false;

  #undef NUM_RESERVED
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

bool Match(Reader *r, char *expect)
{
  u32 len = strlen(expect);
  for (u32 i = 0; i < len; i++) {
    if IsEnd(r->src[r->cur + i]) return false;
    if (r->src[r->cur + i] != expect[i]) return false;
  }
  r->cur += len;
  SkipSpace(r);
  return true;
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

  if (r->cur == start) {
    Error("Expected a symbol at %d\n", r->cur);
  }

  Val symbol = MakeSymbol(&r->src[start], r->cur - start);
  return symbol;
}

Val ParseExpr(Reader *r);

Val ParsePair(Reader *r)
{
  Val head = ParseExpr(r);
  SkipSpace(r);

  if (r->src[r->cur] == '|') {
    r->cur++;
    SkipSpace(r);
    Val tail = ParseExpr(r);
    SkipSpace(r);
    if (r->src[r->cur] != ')') {
      Error("Malformed pair");
    }
    r->cur++;
    SkipSpace(r);
    return MakeList(3, MakeSymbol("make-pair", 9), head, tail);
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
    return MakePair(MakeSymbol("make-tuple", 10), nil);
  }

  Val items = nil;
  u32 count = 0;
  while (r->src[r->cur] != ']') {
    Val item = ParseExpr(r);
    items = MakePair(item, items);
    count++;
    SkipSpace(r);
  }
  r->cur++;
  SkipSpace(r);

  items = Reverse(items);
  return MakePair(MakeSymbol("make-tuple", 10), items);
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

Val ParseDict(Reader *r)
{
  Val pairs = nil;

  while (r->src[r->cur] != '}') {
    Val key;
    if (r->src[r->cur] == '"') {
      key = ParseBinary(r);
    } else {
      key = ParseSym(r);
    }

    SkipSpace(r);
    if (r->src[r->cur] != ':') {
      Error("Expected ':' in dict at %d", r->cur);
    }
    r->cur++;
    SkipSpace(r);
    Val value = ParseExpr(r);
    SkipSpace(r);

    Val pair = MakePair(key, value);
    pairs = MakePair(pair, pairs);
  }
  r->cur++;

  pairs = Reverse(pairs);
  return MakePair(MakeSymbol("make-dict", 9), pairs);
}

Val ParseBlock(Val block, Reader *r)
{
  Val end = MakeSymbol("end", 3);
  SkipSpace(r);

  while (!Eq(Head(block), end)) {
    block = MakePair(ParseExpr(r), block);
    SkipSpace(r);
  }
  return Tail(block);
}

Val ParseKeyword(Val symbol, Reader *r)
{
  Val exp;
  if (Eq(symbol, MakeSymbol("do", 2))) {
    Val block = MakePair(symbol, nil);
    exp = Reverse(ParseBlock(block, r));
  } else if (Eq(symbol, MakeSymbol("def", 3))) {
    Val template = ParseExpr(r);
    SkipSpace(r);
    Val value = ParseExpr(r);
    exp = MakeList(3, symbol, template, value);
  } else if (Eq(symbol, MakeSymbol("module", 6))) {
    Val name = ParseSym(r);
    SkipSpace(r);
    if (!Match(r, "do")) {
      Error("Expected \"do\" after module name at %d", r->cur);
    }
    Val block = MakePair(MakeSymbol("do", 2), nil);
    block = ParseBlock(block, r);
    Val param = MakeSymbol("msg", 3);
    Val fn = MakeList(2, MakeSymbol("lookup", 6), param);
    block = MakePair(fn, block);

    Val body = Reverse(block);

    Val template = MakeList(2, name, param);
    exp = MakeList(3, MakeSymbol("def", 3), template, body);
  } else if (Eq(symbol, MakeSymbol("if", 2))) {
    SkipSpace(r);
    Val predicate = ParseExpr(r);
    Val consequent, alternative;
    if (Match(r, "do")) {
      Val end = MakeSymbol("end", 3);
      Val els = MakeSymbol("else", 4);
      Val block = MakePair(MakeSymbol("do", 2), nil);
      while (!Eq(Head(block), end) && !Eq(Head(block), els)) {
        block = MakePair(ParseExpr(r), block);
        SkipSpace(r);
      }
      consequent = Reverse(Tail(block));
      if (Eq(Head(block), els)) {
        block = MakePair(MakeSymbol("do", 2), nil);
        alternative = ParseBlock(block, r);
      } else {
        alternative = nil;
      }
    } else {
      consequent = ParseExpr(r);
      if (Match(r, "else")) {
        alternative = ParseExpr(r);
      } else {
        alternative = nil;
      }
    }

    exp = MakeList(4, MakeSymbol("if", 2), predicate, consequent, alternative);
  } else {
    exp = symbol;
  }

  SkipSpace(r);
  return exp;
}

Val ParseLookup(Val env, Reader *r)
{
  Val exp = MakeList(2, env, MakePair(MakeSymbol("quote", 5), ParseSym(r)));

  if (Match(r, ".")) {
    return ParseLookup(exp, r);
  } else {
    return exp;
  }
}

Val ParseExpr(Reader *r)
{
  Val expr = nil;

  if (r->src[r->cur] == ':') {
    r->cur++;
    SkipSpace(r);
    Val head = MakeSymbol("quote", 5);
    Val tail = ParseExpr(r);
    expr = MakePair(head, tail);
  } else if (r->src[r->cur] == '(') {
    r->cur++;
    SkipSpace(r);
    Val list = nil;
    if (r->src[r->cur] == ')') {
      r->cur++;
    } else {
      list = ParsePair(r);
    }

    SkipSpace(r);
    if (Match(r, "->")) {
      Val body = ParseExpr(r);
      expr = MakeList(3, MakeSymbol("λ", 2), list, body);
    } else {
      expr = list;
    }
  } else if (r->src[r->cur] == '[') {
    r->cur++;
    SkipSpace(r);
    expr = ParseTuple(r);
  } else if (r->src[r->cur] == '{') {
    r->cur++;
    SkipSpace(r);
    expr = ParseDict(r);
  } else if (r->src[r->cur] == '"') {
    r->cur++;
    expr = ParseBinary(r);
  } else if (IsDigit(r->src[r->cur])) {
    expr = ParseInt(r);
  } else if (IsSymStart(r->src[r->cur])) {
    Val symbol = ParseSym(r);
    if (Match(r, ".")) {
      expr = ParseLookup(symbol, r);
    } else {
      SkipSpace(r);
      expr = ParseKeyword(symbol, r);
    }
  } else {
    Error("Unexpected char 0x%02X at %d", (u8)r->src[r->cur], r->cur);
  }

  SkipSpace(r);
  return expr;
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
