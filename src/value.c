#include "value.h"
#include "symbol.h"

Value nilVal = {OBJ, 0, {0, 0}};

ValType TypeOf(Value value)
{
  return value.type;
}

Value Num(u32 n)
{
  Value v = {NUMBER, n, {0, 0}};
  return v;
}

Value Index(u32 pos)
{
  Value v = {OBJ, pos, {0, 0}};
  return v;
}

u32 AsIndex(Value val)
{
  return val.value;
}

Value Symbol(u32 hash)
{
  Value v = {SYMBOL, hash, {0, 0}};
  return v;
}

u32 AsSymbol(Value val)
{
  return val.value;
}

u32 AsNum(Value val)
{
  return val.value;
}

bool IsNil(Value value)
{
  return value.type == OBJ && value.value == 0;
}

void PrintValue(Value value)
{
  u32 len = LongestSymLength() > 8 ? LongestSymLength() : 8;
  switch (TypeOf(value)) {
  case NUMBER:  printf("% 8d", AsNum(value)); break;
  case OBJ:     printf("% 8d", AsIndex(value)); break;
  case SYMBOL:  PrintSymbol(value, len); break;
  default:      printf("        ");
  }
}
