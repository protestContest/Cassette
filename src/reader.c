#include "reader.h"
#include "string.h"
#include "printer.h"
#include "list.h"
#include "vm.h"

void DumpAST(Value ast);

/* Source
 *
 * The source text is stored as a list of binaries. A source object is a pair whose
 * head is the source text and tail is a cursor location of the current position
 * during parsing. As the source is parsed, the cursor is advanced.
 *
 * GetLine reads an input string into a buffer and creates a binary value trimmed
 * of whitespace.
 */

#define MAX_INPUT_LENGTH 1024
#define END_MARK    '$'

#define MakeCursor(line, col)   MakePair(line, col)
#define CopyCursor(cur)         MakeCursor(LineNum(cur), Column(cur))
#define SetLineNum(cur, i)      SetHead(cur, i)
#define SetColumn(cur, i)       SetTail(cur, i)

#define MakeSource()            MakePair(nil_val, MakeCursor(IndexVal(0), IndexVal(0)))
#define AddLine(line, source)   SetHead(source, MakePair(line, SourceLines(source)))
#define Peek(source)            CharAt(CurrentLine(source), CurrentColumn(source))
#define Advance(source)         SetColumn(SourcePos(source), Incr(CurrentColumn(source)))
#define AdvanceLine(source)     SetLineNum(SourcePos(source), Incr(CurrentLineNum(source)))
#define IsAtEnd(source)         (Peek(source) == END_MARK)
#define SkipWhitespace(source)  do { while (IsSpace(Peek(source)) && !IsAtEnd(source)) Advance(source); } while (0)

Value SourceLine(Value source, Value line_num)
{
  return ListAt(SourceLines(source), line_num);
}

/* Given a source object, this reads in a line of text from the user and appends
 * it to the source.
 */
Value GetLine(Value source)
{
  static char buf[MAX_INPUT_LENGTH];
  fgets(buf, MAX_INPUT_LENGTH, stdin);

  // Set start after all whitespace
  u32 start = 0;
  while (IsSpace(buf[start]) && !IsNewline(buf[start])) {
    start++;
  }

  // Scan to the end, saving the last non-whitespace position
  u32 end = start;
  u32 maybe_end = end;
  while (!IsNewline(buf[maybe_end])) {
    if (!IsSpace(buf[maybe_end])) {
      end = maybe_end + 1;
    }
    maybe_end++;
  }

  // Mark the end
  buf[end++] = END_MARK;

  Value line = MakeBinary(buf, start, end);
  AddLine(line, source);
  SetColumn(SourcePos(source), IndexVal(0));
  return source;
}

/* Tokens
 *
 * A token is a pair whose head is a parsed value and tail is a (start . end)
 * pair, marking the start and end positions in the source.
 */

typedef enum {
  DIGIT,
  SYMCHAR,
  LEFT_PAREN,
  RIGHT_PAREN,
  QUOTE
} TokenType;

#define MakeToken(val, start, end)  MakePair(val, MakePair(start, end))
#define TokenValue(token)           Head(token)
#define SetTokenValue(token, val)   SetHead(token, val)
#define TokenLoc(token)             Tail(token)
#define TokenStart(token)           Head(TokenLoc(token))
#define TokenEnd(token)             Tail(TokenLoc(token))
#define SetTokenStart(token, pos)   SetHead(TokenLoc(token), MakeCursor(LineNum(pos), Column(pos)))
#define SetTokenEnd(token, pos)     SetTail(TokenLoc(token), MakeCursor(LineNum(pos), Column(pos)))

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
  Value start;
  if (LineNum(TokenStart(token)) != CurrentLineNum(source)) {
    start = IndexVal(0);
  } else {
    start = Column(TokenStart(token));
  }

  Value end;
  if (IsNil(TokenEnd(token))) {
    end = CurrentColumn(source);
  } else {
    end = Column(TokenEnd(token));
  }

  printf("  ");
  WriteSlice(CurrentLine(source), IndexVal(0), start);
  BeginUnderline();
  WriteSlice(CurrentLine(source), start, end);
  EndUnderline();
  WriteSlice(CurrentLine(source), end, IndexVal(BinarySize(CurrentLine(source))));

  printf("\n  ");
  Value spaces = CountGraphemes(CurrentLine(source), IndexVal(0), end);
  for (u32 i = 0; i < RawVal(spaces); i++) printf(" ");
  printf("↑\n");
}

/* Token List
 *
 * A token list is a list of tokens that represent open expressions being
 * parsed. Each token in the list represents a list expression in the source.
 * These tokens have a nil end position, since they are currently being parsed.
 * When the list being parsed is closed, it is appended to the parent token list
 * as an item, and popped from the top-level token list.
 */

#define ListToken(list)       Head(list)
#define ParentList(list)      Tail(list)
#define MakeListToken(source) MakeToken(nil_val, CopyCursor(SourcePos(source)), nil_val)
#define CurrentTokens(list)   TokenValue(ListToken(list))

Value BeginTokenList(Value ast, Value source)
{
  Value list = MakeListToken(source);
  ast = MakePair(list, ast);
  return ast;
}

void ListTokenAttach(Value item, Value list_token)
{
  SetTail(item, TokenValue(list_token));
  SetTokenValue(list_token, item);
}

Value AddToken(Value token, Value ast)
{
  Value list_token = ListToken(ast);
  Value list = MakePair(token, TokenValue(list_token));
  SetTokenValue(list_token, list);
  return ast;
}

void ReverseTokenList(Value list_token)
{
  SetTokenValue(list_token, ReverseList(TokenValue(list_token)));
}

Value EndTokenList(Value ast, Value source)
{
  Value parent = ParentList(ast);
  if (IsNil(parent)) {
    SyntaxError("Too many closing parens", source);
  }

  // Set end location of the list
  SetTokenEnd(ListToken(ast), CopyCursor(SourcePos(source)));

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
  Value start = CopyCursor(SourcePos(source));

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

  Value end = CopyCursor(SourcePos(source));
  return MakeToken(NumberVal(n), start, end);
}

Value ParseSymbol(Value source)
{
  Value start = CopyCursor(SourcePos(source));

  while (IsSymchar(Peek(source)) && !IsAtEnd(source)) {
    Advance(source);
  }

  Value end = CopyCursor(SourcePos(source));
  Value symbol = MakeSymbolFrom(CurrentLine(source), Column(start), Column(end));
  Value token = MakeToken(symbol, start, end);
  return token;
}

bool WasQuoted(Value ast)
{
  Value tokens = CurrentTokens(ast);
  if (IsNil(Tail(tokens))) return false;
  Value prev_token = ListToken(Tail(tokens));
  return TokenValue(prev_token) == GetSymbol("quote");
}

Value BeginQuote(Value ast, Value source)
{
  ast = BeginTokenList(ast, source);
  Advance(source);
  Value symbol = CreateSymbol("quote");
  Value quoteToken = MakeToken(symbol, CopyCursor(SourcePos(source)), CopyCursor(SourcePos(source)));
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
    if (WasQuoted(ast)) ast = EndQuote(ast, source);
    return ast;
  }
  case SYMCHAR: {
    Value token = ParseSymbol(source);
    ast = AddToken(token, ast);
    if (WasQuoted(ast)) {
      ast = EndQuote(ast, source);
    }
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
    if (WasQuoted(ast)) {
      ast = EndQuote(ast, source);
    }
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
    DebugToken(Head(CurrentTokens(ast)), source);
  }

  if (IsNil(ParentList(ast))) {
    // No open parens
    SetTokenEnd(ListToken(ast), CopyCursor(SourcePos(source)));
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

Value ReadNext(Value ast, Value source)
{
  source = GetLine(source);
  ast = Parse(source, ast);

  if (IsNil(Tail(ast))) {
    return ast;
  }

  AdvanceLine(source);

  printf(". ");
  return ReadNext(ast, source);
}

Value Read(void)
{
  Value source = MakeSource();
  Value initial_token = MakeToken(nil_val, CopyCursor(SourcePos(source)), nil_val);
  Value ast = MakePair(initial_token, nil_val);

  Prompt();
  return ReadNext(ast, source);
}
