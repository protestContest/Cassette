#include "reader.h"
#include "vm.h"
#include <stdio.h>

#define Peek(reader)        (reader)->src[(reader)->pos]
#define Advance(reader)     (reader)->pos++

#define IsList(v)           IsPair(v)

#define IsEnd(c)            ((c) == '\0')
#define IsSpace(c)          ((c) == ' ' || (c) == '\n' || (c) == '\t')
#define IsDigit(c)          ((c) >= '0' && (c) <= '9')
#define IsNumChar(c)        (IsDigit(c) || (c) == '.')

#define IsSymChar(c)       ((c) >= 0x21 && (c) <= 0x7F && (c) != '(' && (c) != ')' && (c) != '\'')

typedef enum {
  INT,
  NUM,
  SYM,
  LIST,
  QUOTE
} TokenType;

typedef enum {
  SYMCHAR,
  DIGIT,
  OPEN_PAREN,
  CLOSE_PAREN,
  QUOTE_CHAR,
  DOT,
  SPACE
} CharType;

typedef void (*ParseFn)(Reader *);

typedef struct {
  TokenType token_type;
  CharType char_type;
  ParseFn parse;
} ParseRule;

void ReadChar(Reader *reader);
void ParseDigit(Reader *reader);
TokenType CurTokenType(Reader *reader);
CharType CurCharType(Reader *reader);
void EndToken(Reader *reader);
void BeginSym(Reader *reader);
void UpdateSym(Reader *reader);
void UpdateInt(Reader *reader);
void UpdateNum(Reader *reader);
void BeginInt(Reader *reader);
void BeginList(Reader *reader);
void BeginQuote(Reader *reader);
void AddToken(Reader *reader, Val value);
void PushToken(Reader *reader);
void PopToken(Reader *reader);
void Ignore(Reader *reader);
void ConvertToNum(Reader *reader);

#define NUM_RULES 23
static ParseRule rules[NUM_RULES] = {
  {LIST,  SYMCHAR,      &BeginSym},
  {LIST,  DIGIT,        &BeginInt},
  {LIST,  OPEN_PAREN,   &BeginList},
  {LIST,  CLOSE_PAREN,  &EndToken},
  {LIST,  QUOTE_CHAR,   &BeginQuote},
  {LIST,  SPACE,        &Ignore},

  {SYM,   SYMCHAR,      &UpdateSym},
  {SYM,   DIGIT,        &UpdateSym},
  {SYM,   CLOSE_PAREN,  &EndToken},
  {SYM,   SPACE,        &EndToken},

  {QUOTE, SYMCHAR,      &BeginSym},
  {QUOTE, DIGIT,        &BeginInt},
  {QUOTE, OPEN_PAREN,   &BeginList},
  {QUOTE, CLOSE_PAREN,  &PopToken},
  {QUOTE, QUOTE_CHAR,   &BeginSym},
  {QUOTE, SPACE,        &EndToken},

  {INT,   DIGIT,        &UpdateInt},
  {INT,   CLOSE_PAREN,  &EndToken},
  {INT,   SPACE,        &EndToken},
  {INT,   DOT,          &ConvertToNum},

  {NUM,   DIGIT,        &UpdateNum},
  {NUM,   CLOSE_PAREN,  &EndToken},
  {NUM,   SPACE,        &EndToken}
};

Val Read(VM *vm, char *src)
{
  Reader reader = {0, nil_val, nil_val, src, vm};

  while (IsSpace(Peek(&reader))) {
    Advance(&reader);
  }

  while (!IsEnd(Peek(&reader))) {
    Debug("read \"%c\"", Peek(&reader));
    ReadChar(&reader);
    Advance(&reader);
  }

  return Head(reader.vm, reader.root);
}

char *TypeName(TokenType type)
{
  switch (type) {
  case INT:   return "INT";
  case NUM:   return "NUM";
  case LIST:  return "LIST";
  case QUOTE: return "QUOTE";
  case SYM:   return "SYM";
  }
}

void ReadChar(Reader *reader)
{
  if (IsEnd(Peek(reader))) {
    Debug("end");
    return;
  }

  TokenType token_type = CurTokenType(reader);
  CharType char_type = CurCharType(reader);

  for (u32 i = 0; i < NUM_RULES; i++) {
    if (rules[i].token_type == token_type && rules[i].char_type == char_type) {
      rules[i].parse(reader);
      return;
    }
  }

  Error("No rule found for %c", Peek(reader));
}

void Ignore(Reader *reader)
{
  Debug("skip");
}

void EndList(Reader *reader)
{
  PopToken(reader);
}

void EndToken(Reader *reader)
{
  TokenType type = CurTokenType(reader);
  PopToken(reader);
  if (CurTokenType(reader) != type) {
    ReadChar(reader);
  }
}

void BeginSym(Reader *reader)
{
  Debug("create sym");
  char c = Peek(reader);
  Val sym = MakeSymbol(reader->vm, &c, 1);
  AddToken(reader, sym);
  PushToken(reader);
}

void UpdateSym(Reader *reader)
{
  Debug("update sym");

  Val old_bin = SymbolName(reader->vm, reader->token);
  u32 old_length = BinaryLength(reader->vm, old_bin);
  Val bin = MakeBinary(reader->vm, old_length + 1);

  char *old_chars = (char*)BinaryData(reader->vm, old_bin);
  char *new_chars = (char*)BinaryData(reader->vm, bin);

  memcpy(new_chars, old_chars, old_length);
  new_chars[old_length] = Peek(reader);

  reader->token = BinToSymbol(reader->vm, bin);
  Val parent = StackPeek(reader->vm, 0);
  SetHead(reader->vm, parent, reader->token);
}

void BeginInt(Reader *reader)
{
  Debug("create int");

  u32 digit = (u32)(Peek(reader) - '0');
  AddToken(reader, IntVal(digit));
  PushToken(reader);
}

void UpdateInt(Reader *reader)
{
  Debug("update int");

  u32 digit = (u32)(Peek(reader) - '0');
  u32 num = RawVal(reader->token)*10 + digit;
  reader->token = IntVal(num);

  Val parent = StackPeek(reader->vm, 0);
  SetHead(reader->vm, parent, reader->token);
}

void ConvertToNum(Reader *reader)
{
  Debug("convert to num");
  u32 num = RawVal(reader->token);
  reader->token.as_f = (float)num;

  Val parent = StackPeek(reader->vm, 0);
  SetHead(reader->vm, parent, reader->token);
}

void UpdateNum(Reader *reader)
{
  Debug("update num");
  float digit = (float)(Peek(reader) - '0');
  u32 precision = 1;
  char *c = &reader->src[reader->pos];
  while (*c-- != '.') {
    precision *= 10;
  }

  float num = RawVal(reader->token) + digit / (float)precision;
  reader->token = NumVal(num);

  Val parent = StackPeek(reader->vm, 0);
  SetHead(reader->vm, parent, reader->token);
}

void BeginList(Reader *reader)
{
  Debug("begin list");
  AddToken(reader, nil_val);
  PushToken(reader);
}

void BeginQuote(Reader *reader)
{
  Debug("begin quote");
  AddToken(reader, nil_val);
  PushToken(reader);
  AddToken(reader, quote_val);
}

void AddToken(Reader *reader, Val value)
{
  Debug("add token");
  assert(IsList(reader->token));

  if (IsNil(reader->token)) {
    reader->token = MakePair(reader->vm, value, nil_val);
    if (IsNil(reader->root)) {
      reader->root = reader->token;
    } else {
      SetHead(reader->vm, StackPeek(reader->vm, 0), reader->token);
    }
  } else {
    SetTail(reader->vm, reader->token, MakePair(reader->vm, value, Tail(reader->vm, reader->token)));
    reader->token = Tail(reader->vm, reader->token);
  }
}

void PushToken(Reader *reader)
{
  Debug("push token");
  Val value = Head(reader->vm, reader->token);
  StackPush(reader->vm, reader->token);
  reader->token = value;
}

void PopToken(Reader *reader)
{
  Debug("pop token");
  reader->token = StackPop(reader->vm);
}

TokenType CurTokenType(Reader *reader)
{
  if (IsNum(reader->token))   return NUM;
  if (IsInt(reader->token))   return INT;
  if (IsSym(reader->token))   return SYM;

  if (IsPair(reader->token)) {
    Val list_head = (IsStackEmpty(reader->vm)) ? reader->root : Head(reader->vm, StackPeek(reader->vm, 0));
    if (Eq(Head(reader->vm, list_head), quote_val)) {
      return QUOTE;
    } else {
      return LIST;
    }
  }

  Error("Unknown token type");
}

CharType CurCharType(Reader *reader)
{
  char c = Peek(reader);
  if (c == '(')     return OPEN_PAREN;
  if (c == ')')     return CLOSE_PAREN;
  if (c == '\'')    return QUOTE_CHAR;
  if (c == '.')     return DOT;
  if (IsSpace(c))   return SPACE;
  if (IsDigit(c))   return DIGIT;
  if (IsSymChar(c)) return SYMCHAR;

  Error("Unknown char type");
}