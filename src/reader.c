#include "reader.h"
#include "string.h"
#include "printer.h"
#include "list.h"
#include "vm.h"

/* Source
A source object is a pair whose head is a binary value (the text), and the tail
is an index of the current position during parsing. As the source is parsed, the
index is advanced.

GetLine reads an input string into a buffer and creates a binary value trimmed
of whitespace.
*/

#define MAX_INPUT_LENGTH 1024
#define END_MARK    '$'

#define Text(source)            BinaryData(Head(source))
#define Index(source)           Tail(source)
#define Peek(source)            Text(source)[RawVal(Index(source))]
#define Advance(source)         SetTail(source, IndexVal(RawVal(Index(source)) + 1))
#define IsAtEnd(source)         (Peek(source) == END_MARK)
#define SkipWhitespace(source)  do { while (IsSpace(Peek(source)) && !IsAtEnd(source)) Advance(source); } while (0)

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

  Value text = MakeBinary(buf, start, end);
  Value source = MakePair(text, IndexVal(0));

  return source;
}

/* Tokens
A token is a pair whose head is a parsed value and tail is a (start . end) pair,
marking the start and end positions in the source.
*/

typedef enum {
  DIGIT,
  SYMCHAR,
  LEFT_PAREN,
  RIGHT_PAREN
} TokenType;

#define TokenValue(token)       Head(token)
#define TokenLoc(token)         Tail(token)
#define TokenStart(token)       RawVal(Head(TokenLoc(token)))
#define TokenEnd(token)         RawVal(Tail(TokenLoc(token)))
#define IsSymchar(c)      (!IsCtrl(c) && (c) != ' ' && (c) != '(' && (c) != ')' && (c) != '.')

TokenType SniffToken(Value source)
{
  if (IsDigit(Peek(source)))    {return DIGIT;}
  if (Peek(source) == '-')      {return DIGIT;}
  if (IsSymchar(Peek(source)))  {return SYMCHAR;}
  if (Peek(source) == '(')      {return LEFT_PAREN;}
  if (Peek(source) == ')')      {return RIGHT_PAREN;}

  Error("Syntax error: Unexpected character at column %d\n", Index(source));
}

Value MakeToken(Value val, Value start, Value end)
{
  Value loc = MakePair(start, end);
  Value token = MakePair(val, loc);
  return token;
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
  DebugValue(token);

  printf("\n  ");
  for (u32 i = 0; i < CountGraphemes(text, 0, RawVal(Index(source))); i++) printf(" ");
  printf("↑\n");
}

/* List token
A list token is a token whose value is a list of other tokens. A partial list
token (where the end index is unknown) is used while parsing open lists. When a
close paren is found, the end index is set and the list token is added as an
item in the parent list.
*/

#define CurrentList(ast)    Head(ast)

Value BeginTokenList(Value ast, Value source)
{
  Value list = MakeToken(nil_val, Index(source), nil_val);
  Advance(source);
  ast = MakePair(list, ast);
  return ast;
}

Value AddToken(Value token, Value ast, Value source)
{
  ListPush(CurrentList(ast), token);
  DebugToken(token, source);
  return ast;
}

Value EndTokenList(Value ast, Value source)
{
  Advance(source);
  SetTail(TokenLoc(CurrentList(ast)), Index(source));
  SetTail(CurrentList(ast), ReverseList(Tail(CurrentList(ast))));
  AddToken(CurrentList(ast), Tail(ast), source);
  return Tail(ast);
}

/* Parsing
The AST is a stack of unclosed list tokens. The head of the stack is the
"current list". Start with one list token in the stack — this allows us to
ignore parens at the top level.

Proceed by parsing each token:

Check the next token type.
  End of input?
    Return the AST.
  Some parseable value?
    Parse the value into a token.
    Push the token onto the current list.
  Start of a new list?
    Push a new list token onto the stack as a new current list.
  End of a list?
    Reverse the current list.
    Set the list token's end index to the current index.
    Append the current list token as an item in the parent list.
    The parent list is now the current list.
*/

Value ParseNumber(Value source)
{
  Value start = Index(source);

  i32 sign = 1;
  if (Peek(source) == '-') {
    Advance(source);
    sign = -1;
  }

  float n = 0;
  while (IsDigit(Peek(source)) && !IsAtEnd(source)) {
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

Value ParseSymbol(Value source)
{
  Value start = Index(source);
  while (IsSymchar(Peek(source)) && !IsAtEnd(source)) {
    Advance(source);
  }

  Value symbol = CreateSymbol(Text(source), RawVal(start), RawVal(Index(source)));
  Value token = MakeToken(symbol, start, Index(source));
  return token;
}

Value ParseToken(Value source, Value ast)
{
  SkipWhitespace(source);

  if (IsAtEnd(source)) {
    return ast;
  }

  switch (SniffToken(source)) {
  case DIGIT:       return AddToken(ParseNumber(source), ast, source);
  case SYMCHAR:     return AddToken(ParseSymbol(source), ast, source);
  case LEFT_PAREN:  return BeginTokenList(ast, source);
  case RIGHT_PAREN: return EndTokenList(ast, source);
  }
}

Value Parse(Value source, Value ast)
{
  while (!IsAtEnd(source)) {
    ast = ParseToken(source, ast);
  }

  return ast;
}

/* Read
Some source text is read from the input and parsed. This is repeated until the
resulting AST is complete. The AST is incomplete when there is more than one
open list. If the AST is complete, the list is reversed and returned as a top-
level token.
*/

#define Prompt()                printf("⌘ ")

void DumpAST(Value ast, Value source);

Value Read(void)
{
  Value initial_token = MakeToken(nil_val, IndexVal(0), nil_val);
  Value ast = MakePair(initial_token, nil_val);

  Prompt();
  Value source = GetLine();
  ast = Parse(source, ast);

  DumpPairs();

  if (ListLength(ast) > 1) {
    printf("  ");
    source = GetLine();
    ast = Parse(source, ast);
  }

  return ast;
}

/* Dump AST */

void DumpASTList(Value tokens, Value source, u32 indent, u32 lines)
{
  if (IsNil(tokens)) return;

  Value token = Head(tokens);

  BeginRow();
  DebugValue(token);
  TableSep();

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

  printf("%s \"", TypeAbbr(TokenValue(token)));
  ExplicitPrint(Text(Head(source)) + TokenStart(token), TokenEnd(token) - TokenStart(token));
  printf("\"");

  if (IsPair(TokenValue(token))) {
    lines <<= 1;
    lines |= 0x01;
    DumpASTList(TokenValue(token), source, indent + 1, lines);
    lines >>= 1;
  }

  DumpASTList(Tail(tokens), source, indent, lines);
}

void DumpASTRec(Value ast, Value source)
{
  if (IsNil(ast)) return;
  DumpASTList(Head(ast), source, 0, 1);
  DumpASTRec(Tail(ast), source);
}

void DumpAST(Value ast, Value source)
{
  if (IsNil(ast)) {
    EmptyTable("⌥ Syntax Tree");
    return;
  }

  TableTitle("⌥ Syntax Tree", 1, 0);
  DumpASTRec(ast, source);
  TableEnd();
}
