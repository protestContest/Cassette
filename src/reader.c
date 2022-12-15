#include "reader.h"
#include "mem.h"
#include "symbol.h"
#include "string.h"
#include "debug.h"

typedef enum {
  UNKNOWN,
  DIGIT,
  SYMCHAR,
  LEFT_PAREN,
  RIGHT_PAREN,
  END,
  DOT
} TokenType;

#define INPUT_LEN 1024

#define IsSpace(c)    ((c) == ' ' || (c) == '\n' || (c) == '\t' || (c) == '\r')
#define IsDigit(c)    ((c) >= '0' && (c) <= '9')
#define IsAlpha(c)    (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && c <= 'z'))
#define IsSymchar(c)  (!IsSpace(c) && (c) != '(' && (c) != ')' && (c) != '.' && (c) != '\0')
#define IsEnd(c)      ((c) == '\0')

Value ParseRest(char *src, u32 start, Value prev);
Value ParseExpr(char *src, u32 start);
Value ParseNumber(char *src, u32 start);
Value ParseSymbol(char *src, u32 start);
Value ParseList(char *src, u32 start);
Value MakeToken(Value val, u32 start, u32 end);
Value TokenValue(Value token);
u32 TokenStart(Value token);
u32 TokenEnd(Value token);

Value GetLine(Value input);
TokenType SniffToken(char c);
Value Reverse(Value list);

void DebugToken(char *src, Value token, u32 start);

Value Read(void)
{
  Value ast = nil_val;
  printf("⌘ ");
  Value input = GetLine(nil_val);

  ast = Parse(BinaryData(Head(input)));
  // DebugValue(input, 0);

  return MakePair(ast, input);
}

Value Parse(char *src)
{
  Value token = ParseRest(src, 0, nil_val);
  return Reverse(token);
}

Value ParseRest(char *src, u32 cur, Value prev)
{
  while (IsSpace(*(src))) (src)++;
  Value token = ParseExpr(src, cur);

  if (IsNil(token)) return prev;

  cur = TokenEnd(token);
  while (IsSpace(src[cur])) cur++;

  DebugToken(src, token, cur);

  Value item = MakePair(token, prev);
  return ParseRest(src, cur, item);
}

Value ParseExpr(char *src, u32 cur)
{
  switch (SniffToken(src[cur])) {
  case LEFT_PAREN:  return ParseList(src, cur);
  case RIGHT_PAREN: return nil_val;
  case SYMCHAR:     return ParseSymbol(src, cur);
  case DIGIT:       return ParseNumber(src, cur);
  case END:         return nil_val;
  default:
    fprintf(stderr, "Syntax error: Unknown token begins with \"0x%02X\"\n", src[cur]);
    fprintf(stderr, "  %s", src);

    fprintf(stderr, "  ");
    for (u32 i = 0; i < StringLength(src, 0, cur); i++) fprintf(stderr, " ");
    fprintf(stderr, "↑\n");
    exit(1);
  }
}

Value ParseNumber(char *src, u32 cur)
{
  u32 start = cur;

  i32 sign = 1;
  if (src[cur] == '-') {
    cur++;
    sign = -1;
  }

  float n = 0;
  while (IsDigit(src[cur])) {
    i32 d = src[cur++] - '0';
    n = n*10 + sign*d;
  }

  if (src[cur] == '.') {
    cur++;
    u32 div = 1;
    while (IsDigit(src[cur])) {
      i32 d = src[cur++] - '0';
      n += (float)d / div;
      div /= 10;
    }
  }

  return MakeToken(NumberVal(n), start, cur);
}

Value ParseSymbol(char *src, u32 cur)
{
  u32 start = cur;
  while (IsSymchar(src[cur])) cur++;

  Value symbol = CreateSymbol(src, start, cur);
  Value token = MakeToken(symbol, start, cur);
  return token;
}

Value ParseList(char *src, u32 start)
{
  Value items = ParseRest(src, start + 1, nil_val);

  u32 end = TokenEnd(Head(items));
  end++; // close paren

  return MakeToken(Reverse(items), start, end);
}

TokenType SniffToken(char c)
{
  if (c == '.')               return DOT;
  if (c == '-' || IsDigit(c)) return DIGIT;
  if (c == '(')               return LEFT_PAREN;
  if (c == ')')               return RIGHT_PAREN;
  if (c == '\0')              return END;
  if (IsSpace(c))             return UNKNOWN;

  return SYMCHAR;
}

void DebugToken(char *src, Value token, u32 cur)
{
  u32 src_size = strlen(src);

  printf("  ");
  ExplicitPrint(src, TokenStart(token));
  printf("%s", UNDERLINE_START);
  ExplicitPrint(src + TokenStart(token), TokenEnd(token) - TokenStart(token));
  printf("%s", UNDERLINE_END);
  ExplicitPrint(src + TokenEnd(token), src_size - TokenEnd(token));

  printf("    ");
  DebugValue(token, 0);

  printf("\n  ");
  for (u32 i = 0; i < StringLength(src, 0, cur); i++) printf(" ");
  printf("↑\n");
}

void PrintToken(Value token, Value source)
{
  char *src = BinaryData(source);
  printf("%s \"", TypeAbbr(TokenValue(token)));
  ExplicitPrint(src + TokenStart(token), TokenEnd(token) - TokenStart(token));
  printf("\"");
}

void DumpASTRec(Value tokens, Value src, u32 indent, u32 lines)
{
  if (IsNil(tokens)) return;

  Value token = Head(tokens);

  DebugRow();
  DebugValue(token, 4);
  DebugCol();

  if (indent > 1) {
    for (u32 i = 0; i < indent - 1; i++) {
      u32 bit = 1 << (indent - i - 1);
      if (lines & bit) printf("│ ");
      else printf("  ");
    }
  }

  if (indent > 0) {
    if (IsNil(Tail(tokens))) {
      printf("└─");
    } else {
      printf("├─");
    }
  }

  if (IsNil(Tail(tokens))) lines &= 0xFFFE;

  PrintToken(token, Head(src));

  if (IsPair(TokenValue(token))) {
    lines <<= 1;
    lines |= 0x01;
    DumpASTRec(TokenValue(token), src, indent + 1, lines);
    lines >>= 1;
  }

  DumpASTRec(Tail(tokens), src, indent, lines);
}

void DumpAST(Value ast)
{
  Value tokens = Head(ast);

  DebugTable("⌥ Syntax Tree", 0, 0);
  if (IsNil(tokens)) {
    DebugEmpty();
    return;
  }

  DumpASTRec(tokens, Tail(ast), 0, 1);
  EndDebugTable();
}

Value GetLine(Value last_line)
{
  static char buf[1024];
  fgets(buf, INPUT_LEN, stdin);

  u32 start = 0;
  while (IsSpace(buf[start])) start++;
  u32 end = start;
  while (!IsNewline(buf[end])) end++;
  end--;
  while (IsSpace(buf[end])) end--;
  end++;
  buf[end++] = '\0';

  Value line = MakeBinary(buf, start, end);
  return MakePair(line, last_line);
}

Value ReverseWith(Value value, Value end)
{
  if (IsNil(value)) return end;

  Value next = Tail(value);
  SetTail(value, end);
  return ReverseWith(next, value);
}

Value Reverse(Value list)
{
  return ReverseWith(list, nil_val);
}

Value MakeToken(Value val, u32 start, u32 end)
{
  Value loc = MakePair(IndexVal(start), IndexVal(end));
  Value token = MakePair(val, loc);
  return token;
}

Value TokenValue(Value token)
{
  return Head(token);
}

u32 TokenStart(Value token)
{
  return RawVal(Head(Tail(token)));
}

u32 TokenEnd(Value token)
{
  return RawVal(Tail(Tail(token)));
}
