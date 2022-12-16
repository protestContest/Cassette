/*
Parsing algorithm:

Start with an empty stack of "open" lists. The head of this stack is the
"current list".

Push an empty list onto the stack as the initial current list.

Proceed by parsing each token:

Check the next token type.
  Some value?
    Parse the value.
    Append an item onto the current list whose head is the value.
  Start of a new list?
    Push a new list onto the stack as a new current list.
  End of a list?
    Reverse the current list.
    Append the current list as an item in the previous list.
    The previous list is now the current list.
  End of text?
    If the parens are balanced, there should be one list in the stack. Reverse
    it, and you have the root of the AST.
    If there's more than one list in the stack, there are unclosed parentheses.
    Don't reverse it, since more items may appear later (e.g. with more input).
*/

#include "value.h"
#include "mem.h"
#include "string.h"
#include "debug.h"

typedef enum {
  DIGIT,
  LEFT_PAREN,
  RIGHT_PAREN
} TokenType;

#define END_MARK    '$'

#define Text(source)            BinaryData(Head(source))
#define Index(source)           RawVal(Tail(source))
#define Peek(source)            Text(source)[Index(source)]
#define Advance(source)         SetTail(source, IndexVal(Index(source) + 1))
#define IsAtEnd(source)         (Peek(source) == END_MARK)
#define SkipWhitespace(source)  do { while (IsSpace(Peek(source)) && !IsAtEnd(source)) Advance(source); } while (0)
#define TokenStart(token)       RawVal(Head(Tail(token)))
#define TokenEnd(token)         RawVal(Tail(Tail(token)))
#define TokenValue(token)       Head(token)

Value GetLine(void);
Value Parse(Value source);
void DebugToken(Value token, Value source);

Value Read(void)
{
  Value source = MakePair(GetLine(), IndexVal(0));
  return Parse(source);
}

#define MAX_INPUT_LENGTH 1024
Value GetLine(void)
{
  static char buf[MAX_INPUT_LENGTH];
  fgets(buf, MAX_INPUT_LENGTH, stdin);

  u32 start = 0;
  while (IsSpace(buf[start])) { start++; }
  u32 end = start;
  while (!IsNewline(buf[end])) { end++; }
  end--;
  while (IsSpace(buf[end])) { end--; }
  end++;
  buf[end++] = END_MARK;

  printf("line: %d %d\n", start, end);

  Value line = MakeBinary(buf, start, end);
  return line;
}

Value ParseToken(Value source, Value lists);

Value Parse(Value source)
{
  Value lists = MakePair(nil_val, nil_val);

  while (!IsAtEnd(source)) {
    lists = ParseToken(source, lists);
  }

  return lists;
}

TokenType SniffToken(Value source);
Value BeginList(Value source, Value lists);
Value EndList(Value source, Value lists);
Value ParseNumber(Value source);
Value AddToken(Value token, Value source, Value lists);

Value ParseToken(Value source, Value lists)
{
  SkipWhitespace(source);

  if (IsAtEnd(source)) {
    return lists;
  }

  switch (SniffToken(source)) {
  case DIGIT:       return AddToken(ParseNumber(source), source, lists);
  case LEFT_PAREN:  return BeginList(source, lists);
  case RIGHT_PAREN: return EndList(source, lists);
  }
}

TokenType SniffToken(Value source)
{
  char *text = Text(source);
  u32 index = Index(source);

  if (IsDigit(text[index])) {
    return DIGIT;
  }

  switch (text[index]) {
  case '-':       return DIGIT;
  case '(':       return LEFT_PAREN;
  case ')':       return RIGHT_PAREN;
  default:
    Error("Syntax error: Unexpected character at column %d\n", index);
  }
}

Value AddToken(Value token, Value source, Value lists)
{
  DebugToken(token, source);

  Value current_list = Head(lists);
  Value item = MakePair(token, current_list);
  SetHead(lists, item);
  return lists;
}

Value BeginList(Value source, Value lists)
{
  Advance(source);
  return MakePair(nil_val, lists);
}

Value ReverseList(Value list);

Value EndList(Value source, Value lists)
{
  Value prev_list = Tail(lists);

  if (IsNil(prev_list)) {
    Error("Too many trailing parens");
  }

  ReverseList(Head(lists));
  SetTail(lists, Head(prev_list));
  SetHead(prev_list, lists);

  Advance(source);

  return prev_list;
}

Value MakeToken(Value val, u32 start, u32 end);

Value ParseNumber(Value source)
{
  u32 start = Index(source);

  i32 sign = 1;
  if (Peek(source) == '-') {
    Advance(source);
    sign = -1;
  }

  float n = 0;
  while (IsDigit(Peek(source))) {
    i32 d = Peek(source) - '0';
    n = n*10 + sign*d;
    Advance(source);
  }

  if (Peek(source) == '.') {
    Advance(source);
    u32 div = 1;
    while (IsDigit(Peek(source))) {
      i32 d = Peek(source) - '0';
      n += (float)d / div;
      div /= 10;
      Advance(source);
    }
  }

  return MakeToken(NumberVal(n), start, Index(source));
}

Value MakeToken(Value val, u32 start, u32 end)
{
  Value loc = MakePair(IndexVal(start), IndexVal(end));
  Value token = MakePair(val, loc);
  return token;
}

Value ReverseWith(Value value, Value tail)
{
  if (IsNil(value)) return tail;

  Value next = Tail(value);
  SetTail(value, tail);
  return ReverseWith(next, value);
}

Value ReverseList(Value list)
{
  return ReverseWith(list, nil_val);
}

void DebugToken(Value token, Value source)
{
  char *text = Text(source);

  u32 src_size = 0;
  while (text[src_size] != END_MARK) src_size++;
  src_size++;

  printf("  ");
  ExplicitPrint(text, TokenStart(token));
  printf("%s", UNDERLINE_START);
  ExplicitPrint(text + TokenStart(token), TokenEnd(token) - TokenStart(token));
  printf("%s", UNDERLINE_END);
  ExplicitPrint(text + TokenEnd(token), src_size - TokenEnd(token));

  printf("    ");
  DebugValue(token, 0);

  printf("\n  ");
  for (u32 i = 0; i < StringLength(text, 0, Index(source)); i++) printf(" ");
  printf("↑\n");
}
