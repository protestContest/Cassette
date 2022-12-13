#include "value.h"
#include "symbol.h"

Value nilVal = {{OBJECT, 0}, {0, 0}};

ValType TypeOf(Value value)
{
  return value.value.type;
}

Value Num(u32 n)
{
  Value v = {{NUMBER, n}, {0, 0}};
  return v;
}

Value Index(u32 pos)
{
  Value v = {{OBJECT, pos}, {0, 0}};
  return v;
}

u32 AsIndex(Value val)
{
  return val.value.data;
}

Value Symbol(u32 hash)
{
  Value v = {{SYMBOL, hash}, {0, 0}};
  return v;
}

u32 AsSymbol(Value val)
{
  return val.value.data;
}

u32 AsNum(Value val)
{
  return val.value.data;
}

bool IsNil(Value value)
{
  return value.value.type == OBJECT && value.value.data == 0;
}

void PrintValue(Value value, u32 len)
{
  char fmt[16];
  sprintf(fmt, "%% %dd", len);

  switch (TypeOf(value)) {
  case NUMBER:  printf(fmt, AsNum(value)); break;
  case OBJECT:  printf(fmt, AsIndex(value)); break;
  case SYMBOL:  PrintSymbol(value, len); break;
  default:      printf("        ");
  }
}

char *TypeAbbr(Value value)
{
  switch (TypeOf(value)) {
  case NUMBER:  return "№";
  case OBJECT:  return "⚭";
  case SYMBOL:  return "§";
  default:      return "?";
  }
}
