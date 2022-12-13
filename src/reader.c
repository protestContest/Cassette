#include "reader.h"
#include "mem.h"
#include "symbol.h"
#include "list.h"

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

typedef struct InputLine {
  char *data;
  struct InputLine *next;
} InputLine;

typedef struct {
  InputLine *first_line;
  InputLine *last_line;
} Input;

Value ParseRest(char *src, u32 start, Value prev);
Value ParseExpr(char *src, u32 start);
Value ParseNumber(char *src, u32 start);
Value ParseSymbol(char *src, u32 start);
Value ParseList(char *src, u32 start);

void GetLine(Input *input);
TokenType SniffToken(char c);
bool IsSpace(char c);
bool IsDigit(char c);
bool IsAlpha(char c);
bool IsSymchar(char c);
bool IsEnd(char c);

u32 StringLength(char *src, u32 start, u32 end);
void DebugParse(char *src, Value token, u32 start);

TokenList Read(void)
{
  Input input = {NULL, NULL};

  printf("▸ ");
  GetLine(&input);

  TokenList ast;
  while (!feof(stdin)) {
    ast.src = input.last_line->data;
    ast.tokens = Parse(input.last_line->data);

    DumpAST(ast);
    DumpMem();
    DumpSymbols();

    printf("▸ ");
    GetLine(&input);
  }

  return ast;
}

Value Parse(char *src)
{
  while (IsSpace(*src)) src++;
  Value v = ParseRest(src, 0, nilVal);
  return Reverse(v);
}

Value ParseRest(char *src, u32 start, Value prev)
{
  Value token = ParseExpr(src, start);

  if (IsNil(token)) {
    return prev;
  }

  SetTail(token, prev);
  start = token.loc.end;
  while (IsSpace(src[start])) start++;

  DebugParse(src, token, start);

  return ParseRest(src, start, token);
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
    fprintf(stderr, "Syntax error: Unknown token begins with \"0x%02X\"\n", src[start]);
    fprintf(stderr, "%s", src);
    for (u32 i = 0; i < start; i++) printf(" ");
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

  i32 n = 0;
  while (IsDigit(src[end])) {
    end++;
    i32 d = src[end-1] - '0';
    n = n*10 + sign*d;
  }

  Value p = MakePair(Num(n), nilVal);
  p.loc.start = start;
  p.loc.end = end;
  return p;
}

Value ParseSymbol(char *src, u32 start)
{
  u32 end = start;
  while (IsSymchar(src[end])) end++;
  Value v = CreateSymbol(src + start, end - start);
  Value p = MakePair(v, nilVal);
  p.loc.start = start;
  p.loc.end = end;
  return p;
}

Value ParseList(char *src, u32 start)
{
  Value v = ParseRest(src, start + 1, nilVal);

  u32 end = v.loc.end;
  while (IsSpace(src[end])) end++;
  end++;

  Value p = MakePair(Reverse(v), nilVal);
  p.loc.start = start;
  p.loc.end = end;
  return p;
}

TokenType SniffToken(char c)
{
  if (c == '.')                 return DOT;
  if (c == '-' || IsDigit(c))   return DIGIT;
  if (c == '(')                 return LEFT_PAREN;
  if (c == ')')                 return RIGHT_PAREN;
  if (c == '\0')                return END;
  if (IsSpace(c))               return UNKNOWN;
  return SYMCHAR;
}

bool IsSpace(char c)
{
  return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

bool IsDigit(char c)
{
  return c >= '0' && c <= '9';
}

bool IsAlpha(char c)
{
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool IsSymchar(char c)
{
  return !IsSpace(c) && c != '(' && c != ')' && c != '.';
}

bool IsEnd(char c)
{
  return c == '\0';
}

void DebugParse(char *src, Value token, u32 cur)
{
  u32 start = StringLength(src, 0, token.loc.start);
  u32 end = StringLength(src, 0, token.loc.end);
  u32 cur_pos = StringLength(src, 0, cur);

  printf("  ");
  char *c = src;
  while (*c != '\0') {
    if (*c == '\n') printf("␤");
    else if (*c == '\r') printf("␍");
    else printf("%c", *c);
    c++;
  }

  printf("\n  ");
  for (u32 i = 0; i < start; i++) printf(" ");
  for (u32 i = start; i < end; i++) printf("‾");
  for (u32 i = end; i < cur_pos; i++) printf(" ");
  printf("↑\n");
}

void DumpTokensRec(Value token, char *src, u32 indent, bool is_tail)
{
  if (IsNil(token)) return;

  printf("  ");
  if (indent > 0) {
    for (u32 i = 0; i < indent-1; i++) {
      if (is_tail) printf("  ");
      else printf("│ ");
    }
    if (IsNil(Tail(token))) {
      printf("└─");
      is_tail = true;
    } else {
      printf("├─");
    }
  }

  printf("%s \"", TypeAbbr(Head(token)));
  for (u32 i = token.loc.start; i < token.loc.end; i++) {
    if (src[i] == '\n') printf("␤");
    else if (src[i] == '\r') printf("␍");
    else printf("%c", src[i]);
  }
  printf("\"\n");

  if (TypeOf(Head(token)) == OBJECT) {
    DumpTokensRec(Head(token), src, indent + 1, is_tail);
  }

  DumpTokensRec(Tail(token), src, indent, is_tail);
}

void DumpAST(TokenList tokens)
{
  printf("\n  \x1B[4mSyntax Tree\x1B[0m\n");
  DumpTokensRec(tokens.tokens, tokens.src, 0, false);
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
