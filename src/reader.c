#include "reader.h"
#include "string.h"
#include "printer.h"
#include "list.h"
#include "vm.h"

void DumpAST(Value ast);

/* Source
A source object is a pair whose head is a binary value (the text), and the tail
is an index of the current position during parsing. As the source is parsed, the
index is advanced.

GetLine reads an input string into a buffer and creates a binary value trimmed
of whitespace.
*/

#define MAX_INPUT_LENGTH 1024
#define END_MARK    '$'

#define SourceLoc(line, col)    MakePair(IndexVal(line), IndexVal(col))
#define SourceLine(loc)         Head(loc)
#define SourceCol(loc)          Tail(loc)

#define Text(source)            BinaryData(Head(source))
#define Cursor(source)          Tail(source)
#define Line(source)            Head(Cursor(source))
#define Col(source)             Tail(Cursor(source))
#define Peek(source)            Text(source)[RawVal(Col(source))]
#define Advance(source)         SetTail(Cursor(source), IndexVal(RawVal(Col(source)) + 1))
#define IsAtEnd(source)         (Peek(source) == END_MARK)
#define SkipWhitespace(source)  do { while (IsSpace(Peek(source)) && !IsAtEnd(source)) Advance(source); } while (0)



Value MakeLine(char *src)
{
  Value text = MakeString(src);
  BinaryData(text)[strlen(src)] = END_MARK;
  return MakePair(text, SourceLoc(0, 0));
}

Value GetLine(void)
{
  static char buf[MAX_INPUT_LENGTH];
  fgets(buf, MAX_INPUT_LENGTH, stdin);

  u32 start = 0;
  while (IsSpace(buf[start]) && !IsNewline(buf[start])) { start++; }
  u32 end = start;
  while (!IsNewline(buf[end])) { end++; }
  end--;
  while (IsSpace(buf[end])) { end--; }
  end++;
  buf[end++] = END_MARK;

  Value text = MakeBinary(buf, start, end);
  Value source = MakePair(text, SourceLoc(0, 0));

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
#define TokenStart(token)       RawVal(SourceCol(TokenLoc(token)))
#define TokenEnd(token)         RawVal(SourceCol(TokenLoc(token)))
#define IsSymchar(c)      (!IsCtrl(c) && (c) != ' ' && (c) != '(' && (c) != ')' && (c) != '.')

void SyntaxError(const char *message, Value source)
{
  fprintf(stderr, "%d:%d Syntax error: %s\n\n", RawVal(Line(source)), RawVal(Col(source)), message);

  char *text = Text(source);
  u32 size = BinarySize(Head(source));
  fprintf(stderr, "  ");
  for (u32 i = 0; i < size; i++) {
    fprintf(stderr, "%c", text[i]);
  }

  fprintf(stderr, "\n  ");

  for (u32 i = 0; i < RawVal(Col(source)); i++) {
    fprintf(stderr, " ");
  }
  fprintf(stderr, "↑\n");
}

TokenType SniffToken(Value source)
{
  if (IsDigit(Peek(source)))    {return DIGIT;}
  if (Peek(source) == '-')      {return DIGIT;}
  if (IsSymchar(Peek(source)))  {return SYMCHAR;}
  if (Peek(source) == '(')      {return LEFT_PAREN;}
  if (Peek(source) == ')')      {return RIGHT_PAREN;}

  SyntaxError("Unexpected token", source);
  exit(1);
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
  for (u32 i = 0; i < CountGraphemes(text, 0, RawVal(Col(source))); i++) printf(" ");
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
  Value list = MakeToken(nil_val, Cursor(source), nil_val);
  Advance(source);
  ast = MakePair(list, ast);
  return ast;
}

Value AddToken(Value token, Value ast, Value source)
{
  printf("%d:%d\n", Line(source), Col(source));
  ListPush(CurrentList(ast), token);
  // DebugToken(token, source);
  return ast;
}

Value EndTokenList(Value ast, Value source)
{
  Value parent = Tail(ast);
  if (IsNil(parent)) {
    SyntaxError("Too many closing parens", source);
  }

  Advance(source);
  SetTail(TokenLoc(CurrentList(ast)), Cursor(source));
  SetHead(CurrentList(ast), ReverseList(TokenValue(CurrentList(ast))));
  SetTail(ast, TokenValue(CurrentList(parent)));
  SetHead(CurrentList(parent), ast);
  // DebugToken(CurrentList(ast), source);
  return parent;
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
  Value start = Cursor(source);

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

  return MakeToken(NumberVal(n), start, Cursor(source));
}

Value ParseSymbol(Value source)
{
  Value start = Cursor(source);
  while (IsSymchar(Peek(source)) && !IsAtEnd(source)) {
    Advance(source);
  }

  Value end = Cursor(source);
  Value symbol = CreateSymbol(Text(source), RawVal(Tail(start)), RawVal(Tail(end)));
  Value token = MakeToken(symbol, start, end);
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

  if (IsNil(Tail(ast))) {
    SetTail(TokenLoc(CurrentList(ast)), Cursor(source));
    SetHead(CurrentList(ast), ReverseList(TokenValue(CurrentList(ast))));
  }

  return ast;
}

/* Dump AST */

void DumpTokenList(Value token_list, u32 indent, u32 lines)
{
  if (IsNil(token_list)) return;

  Value token = Head(token_list);
  bool is_last_item = IsNil(Tail(token_list));

  // Print token pointer
  BeginRow();
  printf("%s ", TypeAbbr(token));
  TableItem(token, 4);
  TableSep();

  // Print graph lines
  for (u32 i = 0; i < indent; i++) {
    if (i == indent - 1) {
      if (!is_last_item) {
        printf("├ ");
      } else {
        printf("└ ");
      }
    } else {
      u32 bit = 1 << (indent - i - 1);
      if (lines & bit) {
        printf("│ ");
      } else {
        printf("  ");
      }
    }
  }

  // Print value
  printf("%s ", TypeAbbr(TokenValue(token)));
  PrintValue(TokenValue(token));

  if (is_last_item) {
    lines &= ~0x1;
  }

  if (IsPair(TokenValue(token))) {
    u32 sub_lines = (lines << 1) | 0x1;
    DumpTokenList(TokenValue(token), indent + 1, sub_lines);
  }

  DumpTokenList(Tail(token_list), indent, lines);
}

void DumpAST(Value ast)
{
  if (IsNil(ast)) {
    EmptyTable("⌥ Syntax Tree");
    return;
  }

  TableTitle("⌥ Syntax Tree", 1, 0);
  DumpTokenList(ast, 0, 0);
  TableEnd();
}

/* Read
Some source text is read from the input and parsed. This is repeated until the
resulting AST is complete. The AST is incomplete when there is more than one
open list. If the AST is complete, the list is reversed and returned as a top-
level token.
*/

#define Prompt()                printf("⌘ ")

Value ReadNext(Value ast)
{
  Value source = GetLine();
  ast = Parse(source, ast);

  if (IsNil(Tail(ast))) {
    return ast;
  }

  printf("  ");
  return ReadNext(ast);
}

Value Read(void)
{
  Value initial_token = MakeToken(nil_val, IndexVal(0), nil_val);
  Value ast = MakePair(initial_token, nil_val);

  Prompt();
  return ReadNext(ast);
}
