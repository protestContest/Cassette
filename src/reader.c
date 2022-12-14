#include "reader.h"
#include "mem.h"
#include "symbol.h"
#include "string.h"

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

Value ParseRest(char *src, u32 start, Value prev);
Value ParseExpr(char *src, u32 start);
Value ParseNumber(char *src, u32 start);
Value ParseSymbol(char *src, u32 start);
Value ParseList(char *src, u32 start);
Value MakeToken(Value val, u32 start, u32 end);
Value TokenValue(Value token);
u32 TokenStart(Value token);
u32 TokenEnd(Value token);

void GetLine(Input *input);
TokenType SniffToken(char c);
bool IsSpace(char c);
bool IsDigit(char c);
bool IsAlpha(char c);
bool IsSymchar(char c);
bool IsEnd(char c);
Value Reverse(Value list);

void DebugParse(char *src, Value token, u32 start);
void DumpAST(Value tokens, char *src);

Value Read(Input *input)
{
  Value ast = nilVal;
  printf("⌘ ");
  GetLine(input);

  while (!feof(stdin)) {
    char *src = input->last_line->data;
    while (IsSpace(*(src))) (src)++;

    ast = Parse(src);

    DumpAST(ast, src);
    DumpPairs();
    DumpSymbols();

    printf("⌘ ");
    GetLine(input);
  }

  return ast;
}

Value Parse(char *src)
{
  Value token = ParseRest(src, 0, nilVal);
  return Reverse(token);
}

Value ParseRest(char *src, u32 start, Value prev)
{
  Value token = ParseExpr(src, start);

  if (IsNil(token)) return prev;

  start = TokenEnd(token);
  while (IsSpace(src[start])) start++;

  DebugParse(src, token, start);

  Value item = MakePair(token, prev);
  return ParseRest(src, start, item);
}

Value ParseExpr(char *src, u32 start)
{
  switch (SniffToken(src[start])) {
  case RIGHT_PAREN: return nilVal;
  case END:         return nilVal;
  case DIGIT:       return ParseNumber(src, start);
  case SYMCHAR:     return ParseSymbol(src, start);
  case LEFT_PAREN:  return ParseList(src, start);
  default:
    fprintf(stderr, "Syntax error: Unknown token begins with \"0x%02X\"\n",
            src[start]);
    fprintf(stderr, "%s", src);
    for (u32 i = 0; i < start; i++)
      printf(" ");
    printf("^\n");
    exit(1);
  }
}

Value ParseNumber(char *src, u32 start)
{
  u32 end = start;

  i32 sign = 1;
  if (src[end] == '-') {
    end++;
    sign = -1;
  }

  float n = 0;
  while (IsDigit(src[end])) {
    i32 d = src[end++] - '0';
    n = n*10 + sign*d;
  }

  if (src[end] == '.') {
    end++;
    u32 div = 1;
    while (IsDigit(src[end])) {
      i32 d = src[end++] - '0';
      n += (float)d / div;
      div /= 10;
    }
  }

  return MakeToken(NumVal(n), start, end);
}

Value ParseSymbol(char *src, u32 start)
{
  u32 end = start;
  while (IsSymchar(src[end])) end++;

  Value symbol = CreateSymbol(src + start, end - start);
  return MakeToken(symbol, start, end);
}

Value ParseList(char *src, u32 start)
{
  Value items = ParseRest(src, start + 1, nilVal);

  u32 end = TokenEnd(Head(items));
  while (IsSpace(src[end])) end++;
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

bool IsSpace(char c) { return c == ' ' || c == '\n' || c == '\t' || c == '\r'; }
bool IsDigit(char c) { return c >= '0' && c <= '9'; }
bool IsAlpha(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
bool IsSymchar(char c) { return !IsSpace(c) && c != '(' && c != ')' && c != '.'; }
bool IsEnd(char c) { return c == '\0'; }

void DebugParse(char *src, Value token, u32 cur)
{
  u32 start = StringLength(src, 0, TokenStart(token));
  u32 end = StringLength(src, 0, TokenEnd(token));
  u32 cur_pos = StringLength(src, 0, cur);

  printf("  ");
  char *c = src;
  while (*c != '\0') {
    if (*c == '\n') printf("␤");
    else if (*c == '\r') printf("␍");
    else printf("%c", *c);
    c++;
  }

  printf("    ");
  PrintValue(token, 0);

  printf("\n  ");
  for (u32 i = 0; i < start; i++) printf(" ");
  for (u32 i = start; i < end; i++) printf("‾");
  for (u32 i = end; i < cur_pos; i++) printf(" ");
  printf("↑\n");
}

void PrintToken(Value token, char *src)
{
  printf("%s \"", TypeAbbr(TokenValue(token)));
  for (u32 i = TokenStart(token); i < TokenEnd(token); i++) {
    if (src[i] == '\n') printf("␤");
    else if (src[i] == '\r') printf("␍");
    else printf("%c", src[i]);
  }
  printf("\"");
}

void DumpASTRec(Value tokens, char *src, u32 indent, u32 lines)
{
  if (IsNil(tokens)) return;

  Value token = Head(tokens);

  printf("  ");
  PrintValue(token, 2);
  printf(" │ ");

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

  PrintToken(token, src);
  printf("\n");

  if (IsPair(TokenValue(token))) {
    lines <<= 1;
    lines |= 0x01;
    DumpASTRec(TokenValue(token), src, indent + 1, lines);
    lines >>= 1;
  }

  DumpASTRec(Tail(tokens), src, indent, lines);
}

void DumpAST(Value tokens, char *src)
{
  printf("  \x1B[4mSyntax Tree\x1B[0m\n");
  DumpASTRec(tokens, src, 0, 1);
  printf("\n");
}

void GetLine(Input *input)
{
  static char buf[1024];
  fgets(buf, INPUT_LEN, stdin);

  InputLine *line = malloc(sizeof(InputLine));
  line->data = malloc(strlen(buf) + 1);
  strcpy(line->data, buf);
  line->next = NULL;

  if (input->first_line == NULL) {
    input->first_line = line;
  } else {
    input->last_line->next = line;
  }
  input->last_line = line;
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
  return ReverseWith(list, nilVal);
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
  return AsNum(Head(Tail(token)));
}

u32 TokenEnd(Value token)
{
  return AsNum(Tail(Tail(token)));
}
