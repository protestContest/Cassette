#include "reader.h"
#include "string.h"
#include "printer.h"
#include "list.h"
#include "vm.h"

void DumpAST(Value ast);

/* Source
A source object is a pair whose head is a binary value (the text), and the tail
is a cursor of the current position during parsing. As the source is parsed, the
cursor is advanced.

GetLine reads an input string into a buffer and creates a binary value trimmed
of whitespace.
*/

#define MAX_INPUT_LENGTH 1024
#define END_MARK    '\0'

#define SourceLoc(line, col)    MakePair(line, col)
#define SetCol(loc, i)          SetTail(loc, i)

#define MakeSource(text)        MakePair(text, SourceLoc(0, 0))
#define CurLine(source)         RawVal(Line(Cursor(source)))
#define CurCol(source)          RawVal(Col(Cursor(source)))
#define Peek(source)            BinaryData(Text(source))[CurCol(source)]
#define Advance(source)         SetCol(Cursor(source), IndexVal(CurCol(source) + 1))
#define IsAtEnd(source)         (Peek(source) == END_MARK)
#define SkipWhitespace(source)  do { while (IsSpace(Peek(source)) && !IsAtEnd(source)) Advance(source); } while (0)

Value MakeLine(char *src)
{
  Value text = CreateBinary(src);
  BinaryData(text)[strlen(src)] = END_MARK;
  return MakeSource(text);
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

  Value text = CreateBinary(buf);
  return MakeSource(text);
}

/* Tokens
A token is a pair whose head is a parsed value and tail is a (start . end) pair,
marking the start and end positions in the source.
*/

typedef enum {
  DIGIT,
  SYMCHAR,
  LEFT_PAREN,
  RIGHT_PAREN,
  QUOTE
} TokenType;

#define MakeToken(val, start, end)  MakePair(val, SourceLoc(start, end))
#define TokenValue(token)           Head(token)
#define SetTokenValue(token, val)   SetHead(token, val)
#define TokenLoc(token)             Tail(token)
#define TokenStart(token)           Head(TokenLoc(token))
#define TokenEnd(token)             Tail(TokenLoc(token))

#define IsSymchar(c)      (!IsCtrl(c) && (c) != ' ' && (c) != '(' && (c) != ')' && (c) != '.')

TokenType SniffToken(Value source)
{
  if (IsDigit(Peek(source)))    {return DIGIT;}
  if (Peek(source) == '-')      {return DIGIT;}
  if (IsSymchar(Peek(source)))  {return SYMCHAR;}
  if (Peek(source) == '(')      {return LEFT_PAREN;}
  if (Peek(source) == ')')      {return RIGHT_PAREN;}
  if (Peek(source) == '\'')     {return QUOTE;}

  SyntaxError("Unexpected token", source);
  exit(1);
}

void DebugToken(Value token, Value source)
{
  Value start = Col(TokenStart(token));
  Value end = Col(TokenEnd(token));

  if (IsNil(start) || IsNil(end) || RawVal(start) > RawVal(end)) {
    Error("Bad token location\n");
  }

  u32 src_size = BinarySize(Text(source));

  if (src_size > 100) {
    Error("Suspicious token size\n");
  }

  printf("  ");
  WriteSlice(Text(source), IndexVal(0), start);
  BeginUnderline();
  WriteSlice(Text(source), start, end);
  EndUnderline();
  WriteSlice(Text(source), end, IndexVal(src_size));

  printf("    ");
  DebugValue(token);

  printf("\n  ");
  for (u32 i = 0; i < CountGraphemes(Text(source), IndexVal(0), Col(Cursor(source))); i++) printf(" ");
  printf("↑\n");
}

/* List token
A list token is a token whose value is a list of other tokens. A partial list
token (where the end index is unknown) is used while parsing open lists. When a
close paren is found, the end index is set and the list token is added as an
item in the parent list.
*/

#define ListToken(list)       Head(list)
#define ParentList(list)      Tail(list)
#define MakeListToken(source) MakeToken(nil_val, Cursor(source), nil_val)
#define TokenList(list_token) TokenValue(list_token)

Value BeginTokenList(Value ast, Value source)
{
  Value list = MakeListToken(source);
  ast = MakePair(list, ast);
  return ast;
}

void ListTokenPush(Value list_token, Value token)
{
  Value list = MakePair(token, TokenList(list_token));
  SetTokenValue(list_token, list);
}

void ListTokenAttach(Value item, Value list_token)
{
  SetTail(item, TokenList(list_token));
  SetTokenValue(list_token, item);
}

Value AddToken(Value token, Value ast)
{
  ListTokenPush(ListToken(ast), token);
  return ast;
}

void ReverseTokenList(Value list_token)
{
  SetTokenValue(list_token, ReverseList(TokenList(list_token)));
}

Value EndTokenList(Value ast, Value source)
{
  Value parent = ParentList(ast);
  if (IsNil(parent)) {
    SyntaxError("Too many closing parens", source);
  }

  // Set end location of the list
  SetCol(TokenLoc(ListToken(ast)), Col(Cursor(source)));

  // Reverse the current list
  ReverseTokenList(ListToken(ast));

  // Append the current list token as an item in the parent list
  ListTokenAttach(ast, ListToken(parent));

  // Parent is the new current list
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
  Value start = SourceLoc(Line(Cursor(source)), Col(Cursor(source)));

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

  Value end = SourceLoc(Line(Cursor(source)), Col(Cursor(source)));
  return MakeToken(NumberVal(n), start, end);
}

Value ParseSymbol(Value source)
{
  u32 start = CurCol(source);
  while (IsSymchar(Peek(source)) && !IsAtEnd(source)) {
    Advance(source);
  }

  u32 end = CurCol(source);
  Value symbol = MakeSymbolFrom(Text(source), start, end);
  Value line = Line(Cursor(source));
  Value token = MakeToken(symbol, SourceLoc(line, IndexVal(start)), SourceLoc(line, IndexVal(end)));
  return token;
}

Value ParseToken(Value source, Value ast);

bool InQuotation(Value ast)
{
  Value first_token = Head(TokenList(ListToken(ast)));
  return TokenValue(first_token) == GetSymbol("quote");
}

Value BeginQuote(Value ast, Value source)
{
  BeginTokenList(ast, source);
  Advance(source);
  Value symbol = CreateSymbol("quote");
  Value quoteToken = MakeToken(symbol, Cursor(source), Cursor(source));
  return AddToken(quoteToken, ast);
}

Value EndQuote(Value ast, Value source)
{
  return EndTokenList(ast, source);
}

Value ParseToken(Value source, Value ast)
{
  SkipWhitespace(source);

  if (IsAtEnd(source)) {
    return ast;
  }

  switch (SniffToken(source)) {
  case DIGIT: {
    Value token = ParseNumber(source);
    ast = AddToken(token, ast);
    if (InQuotation(ast)) ast = EndQuote(ast, source);
    return ast;
  }
  case SYMCHAR: {
    Value token = ParseSymbol(source);
    ast = AddToken(token, ast);
    if (InQuotation(ast)) ast = EndQuote(ast, source);
    return ast;
  }
  case LEFT_PAREN: {
    ast = BeginTokenList(ast, source);
    Advance(source);
    return ast;
  }
  case RIGHT_PAREN: {
    Advance(source);
    ast = EndTokenList(ast, source);
    if (InQuotation(ast)) ast = EndQuote(ast, source);
    return ast;
  }
  case QUOTE: {
    ast = BeginQuote(ast, source);
    Advance(source);
    return ast;
  }
  }
}

Value Parse(Value source, Value ast)
{
  while (!IsAtEnd(source)) {
    ast = ParseToken(source, ast);
  }

  if (IsNil(ParentList(ast))) {
    // No open parens
    SetCol(TokenLoc(ListToken(ast)), Col(Cursor(source)));
    ReverseTokenList(ListToken(ast));
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

  printf(". ");
  return ReadNext(ast);
}

Value Read(void)
{
  Value initial_token = MakeToken(nil_val, IndexVal(0), nil_val);
  Value ast = MakePair(initial_token, nil_val);

  Prompt();
  return ReadNext(ast);
}
