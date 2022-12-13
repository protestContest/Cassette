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

Value ParseRest(char *src, u32 start, Value prev);
Value ParseExpr(char *src, u32 start);
Value ParseNumber(char *src, u32 start);
Value ParseSymbol(char *src, u32 start);
Value ParseList(char *src, u32 start);

TokenType SniffToken(char c);
bool IsSpace(char c);
bool IsDigit(char c);
bool IsAlpha(char c);
bool IsSymchar(char c);
bool IsEnd(char c);

void DebugParse(char *src, Value token, u32 start);

TokenList Read(void)
{
  u32 input_len = 1024;
  char *input = malloc(sizeof(char)*input_len);

  printf("> ");
  fgets(input, input_len, stdin);

  TokenList tokens = {Parse(input), input};
  return tokens;
}

Value Parse(char *src)
{
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

void DebugParse(char *src, Value token, u32 start)
{
  printf("%s", src);
  for (u32 i = 0; i < token.loc.start; i++) printf(" ");
  for (u32 i = token.loc.start; i < token.loc.end; i++) printf("`");
  for (u32 i = token.loc.end; i < start; i++) printf(" ");
  printf("^\n");
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
  return IsAlpha(c) || c == '-' || c == '_' || c == '?' || c == '!';
}

bool IsEnd(char c)
{
  return c == '\0';
}

void PrintTokensRec(Value token, char *src, u32 indent)
{
  if (IsNil(token)) return;

  char *type = "";
  switch (TypeOf(Head(token))) {
  case NUMBER:      type = "NUM "; break;
  case SYMBOL:      type = "SYM "; break;
  case OBJ:         type = "OBJ "; break;
  default: break;
  }

  for (u32 i = 0; i < indent; i++) printf("  ");
  printf("%s\t\"", type);
  for (u32 i = token.loc.start; i < token.loc.end; i++) {
    printf("%c", src[i]);
  }
  printf("\"\n");

  if (TypeOf(Head(token)) == OBJ) {
    PrintTokensRec(Head(token), src, indent + 1);
  }

  PrintTokensRec(Tail(token), src, indent);
}

void PrintTokens(TokenList tokens)
{
  printf("AST:\n");
  PrintTokensRec(tokens.tokens, tokens.src, 0);
}
