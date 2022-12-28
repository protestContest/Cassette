#include "reader.h"
#include "vm.h"
#include "list.h"

#define Peek(reader)        (reader)->src[(reader)->pos]
#define Advance(reader)     (reader)->pos++

#define IsQuote(vm, v)      Eq(Head(vm, Tail(vm, Tail(vm, v))), quote_val)
#define IsList(vm, v)       (IsPair(v) && !IsQuote(vm, v))
#define IsEnd(c)            ((c) == '\0' || (c) == EOF)
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
  SPACE,
  END
} CharType;

typedef void (*ParseFn)(Reader *);

typedef struct {
  TokenType token_type;
  CharType char_type;
  ParseFn parse;
} ParseRule;

void ParseChar(Reader *reader);

void Ignore(Reader *reader);
void EndToken(Reader *reader);

void BeginList(Reader *reader);
void EndList(Reader *reader);
void BeginQuote(Reader *reader);
void EndQuote(Reader *reader);
void BeginSym(Reader *reader);
void EndSym(Reader *reader);
void BeginInt(Reader *reader);
void UpdateInt(Reader *reader);
void ConvertToNum(Reader *reader);
void UpdateNum(Reader *reader);

void AddToken(Reader *reader, Val value);
void PushToken(Reader *reader);
void PopToken(Reader *reader);

TokenType CurTokenType(Reader *reader);
CharType CurCharType(Reader *reader);
char *TokenTypeName(TokenType type);
char *CharTypeName(CharType type);

#define NUM_RULES 28
static ParseRule rules[NUM_RULES] = {
  {LIST,  SYMCHAR,      &BeginSym},
  {LIST,  DIGIT,        &BeginInt},
  {LIST,  OPEN_PAREN,   &BeginList},
  {LIST,  CLOSE_PAREN,  &EndList},
  {LIST,  QUOTE_CHAR,   &BeginQuote},
  {LIST,  SPACE,        &Ignore},
  {LIST,  END,          &Ignore},

  {SYM,   SYMCHAR,      &Ignore},
  {SYM,   DIGIT,        &Ignore},
  {SYM,   CLOSE_PAREN,  &EndSym},
  {SYM,   SPACE,        &EndSym},
  {SYM,   END,          &EndSym},

  {QUOTE, SYMCHAR,      &BeginSym},
  {QUOTE, DIGIT,        &BeginInt},
  {QUOTE, OPEN_PAREN,   &BeginList},
  {QUOTE, CLOSE_PAREN,  &EndQuote},
  {QUOTE, QUOTE_CHAR,   &BeginSym},
  {QUOTE, SPACE,        &EndQuote},
  {QUOTE, END,          &EndQuote},

  {INT,   DIGIT,        &UpdateInt},
  {INT,   CLOSE_PAREN,  &EndToken},
  {INT,   SPACE,        &EndToken},
  {INT,   DOT,          &ConvertToNum},
  {INT,   END,          &EndToken},

  {NUM,   DIGIT,        &UpdateNum},
  {NUM,   CLOSE_PAREN,  &EndToken},
  {NUM,   SPACE,        &EndToken},
  {NUM,   END,          &EndToken}
};

Val Read(VM *vm, char *src)
{
  Reader reader = {0, nil_val, src, vm};

  while (IsSpace(Peek(&reader))) {
    Advance(&reader);
  }

  reader.token = MakeLoop(vm);
  Val root = reader.token;

  do {
    Debug(READ, "read \"%c\"", Peek(&reader));
    ParseChar(&reader);
    Advance(&reader);
  } while (!IsEnd(Peek(&reader)) && !IsStackEmpty(vm));

  return Head(vm, Tail(vm, root));
}

Val ReadFile(VM *vm, char *path)
{
  FILE *file = fopen(path, "r");
  if (!file) {
    Error("Could not open file \"%s\"", path);
  }

  fseek(file, 0, SEEK_END);
  u32 filesize = ftell(file);
  rewind(file);
  char src[filesize];
  fread(src, filesize, 1, file);

  Reader reader = {0, nil_val, src, vm};

  while (IsSpace(Peek(&reader))) {
    Advance(&reader);
  }

  Val root = MakeLoop(vm);
  root = LoopAppend(vm, root, do_val);

  while (IsSpace(Peek(&reader))) {
    Advance(&reader);
  }

  while (!IsEnd(Peek(&reader))) {
    reader.token = MakeLoop(vm);

    do {
      Debug(READ, "read \"%c\"", Peek(&reader));
      ParseChar(&reader);
      Advance(&reader);
    } while (!IsStackEmpty(vm));

    root = LoopAppend(vm, root, Head(vm, reader.token));

    while (IsSpace(Peek(&reader))) {
      Advance(&reader);
    }
  }

  return CloseLoop(vm, root);
}

void ParseChar(Reader *reader)
{
  TokenType token_type = CurTokenType(reader);
  CharType char_type = CurCharType(reader);

  for (u32 i = 0; i < NUM_RULES; i++) {
    if (rules[i].token_type == token_type && rules[i].char_type == char_type) {
      rules[i].parse(reader);
      return;
    }
  }

  Error("No rule found for %c in %s", Peek(reader), TokenTypeName(CurTokenType(reader)));
}

void Ignore(Reader *reader)
{
  Debug(READ, "skip");
}

void EndToken(Reader *reader)
{
  TokenType type = CurTokenType(reader);
  PopToken(reader);
  if (CurTokenType(reader) != type) {
    ParseChar(reader);
  }
}

void BeginList(Reader *reader)
{
  Debug(READ, "begin list");
  AddToken(reader, MakeLoop(reader->vm));
  PushToken(reader);
}

void EndList(Reader *reader)
{
  Debug(READ, "end list");
  SetTail(reader->vm, reader->token, nil_val);
  PopToken(reader);
  SetHead(reader->vm, reader->token, Tail(reader->vm, Head(reader->vm, reader->token)));
  if (!IsList(reader->vm, reader->token)) {
    ParseChar(reader);
  }
}

void BeginQuote(Reader *reader)
{
  Debug(READ, "begin quote");
  BeginList(reader);
  AddToken(reader, quote_val);
}

void EndQuote(Reader *reader)
{
  Debug(READ, "end quote");
  bool was_list = IsList(reader->vm, Head(reader->vm, reader->token));

  SetTail(reader->vm, reader->token, nil_val);
  PopToken(reader);
  SetHead(reader->vm, reader->token, Tail(reader->vm, Head(reader->vm, reader->token)));

  if (!was_list) {
    ParseChar(reader);
  }
}

void BeginSym(Reader *reader)
{
  Debug(READ, "begin sym");
  Val sym = MakeSymbol(reader->vm, "_sym_", 5);
  AddToken(reader, sym);
  PushToken(reader);
}

void EndSym(Reader *reader)
{
  Debug(READ, "end sym");
  u32 start = reader->pos;
  while (&reader->src[start] > reader->src && IsSymChar(reader->src[start-1])) start--;
  Val sym = MakeSymbol(reader->vm, reader->src + start, reader->pos - start);

  PopToken(reader);
  SetHead(reader->vm, reader->token, sym);
  ParseChar(reader);
}

void BeginInt(Reader *reader)
{
  Debug(READ, "create int");

  u32 digit = (u32)(Peek(reader) - '0');
  AddToken(reader, IntVal(digit));
  PushToken(reader);
}

void UpdateInt(Reader *reader)
{
  Debug(READ, "update int");

  u32 digit = (u32)(Peek(reader) - '0');
  u32 num = RawVal(reader->token)*10 + digit;
  reader->token = IntVal(num);

  Val parent = StackPeek(reader->vm, 0);
  SetHead(reader->vm, parent, reader->token);
}

void ConvertToNum(Reader *reader)
{
  Debug(READ, "convert to num");
  u32 num = RawVal(reader->token);
  reader->token.as_f = (float)num;

  Val parent = StackPeek(reader->vm, 0);
  SetHead(reader->vm, parent, reader->token);
}

void UpdateNum(Reader *reader)
{
  Debug(READ, "update num");
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

void AddToken(Reader *reader, Val value)
{
  Debug(READ, "add token");
  reader->token = LoopAppend(reader->vm, reader->token, value);
}

void PushToken(Reader *reader)
{
  Debug(READ, "push token");
  Val value = Head(reader->vm, reader->token);
  StackPush(reader->vm, reader->token);
  reader->token = value;
}

void PopToken(Reader *reader)
{
  Debug(READ, "pop token");
  reader->token = StackPop(reader->vm);
}

TokenType CurTokenType(Reader *reader)
{
  if (IsNum(reader->token))   return NUM;
  if (IsInt(reader->token))   return INT;
  if (IsSym(reader->token))   return SYM;

  if (IsPair(reader->token)) {
    if (IsQuote(reader->vm, reader->token)) {
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
  if (IsEnd(c))     return END;

  Error("Unknown char type");
}


char *TokenTypeName(TokenType type)
{
  switch (type) {
  case INT:   return "INT";
  case NUM:   return "NUM";
  case LIST:  return "LIST";
  case QUOTE: return "QUOTE";
  case SYM:   return "SYM";
  }
}

char *CharTypeName(CharType type)
{
  switch (type) {
  case SYMCHAR:     return "SYMCHAR";
  case DIGIT:       return "DIGIT";
  case OPEN_PAREN:  return "OPEN_PAREN";
  case CLOSE_PAREN: return "CLOSE_PAREN";
  case QUOTE_CHAR:  return "QUOTE_CHAR";
  case DOT:         return "DOT";
  case SPACE:       return "SPACE";
  case END:         return "END";
  }
}
