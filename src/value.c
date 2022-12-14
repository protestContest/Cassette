#include "value.h"
#include "symbol.h"
#include "string.h"

Value nilVal = {NIL, 0};
Value trueVal = {TRUE, 0};
Value falseVal = {FALSE, 0};

ValType TypeOf(Value value)
{
  return value.type;
}

Value IndexVal(u32 pos)
{
  Value v = {INDEX, pos};
  return v;
}

u32 AsIndex(Value val)
{
  return val.data;
}

Value SymbolVal(u32 hash)
{
  Value v = {SYMBOL, hash};
  return v;
}

u32 AsSymbol(Value val)
{
  return val.data;
}

Value NumVal(float n)
{
  Value v = {NUMBER, n};
  return v;
}

float AsNum(Value val)
{
  return val.data;
}

Value VecVal(u32 pos)
{
  Value v = {VECTOR, pos};
  return v;
}

u32 AsVec(Value val)
{
  return val.data;
}

Value PairVal(u32 p)
{
  Value v = {PAIR, p};
  return v;
}

u32 AsPair(Value v)
{
  return v.data;
}

void PrintInt(u32 n, u32 len)
{
  char fmt[16];
  if (len == 0) sprintf(fmt, "%%u");
  else sprintf(fmt, "%% %du", len);
  printf(fmt, n);
}

void PrintStr(Value value, char *str, u32 len)
{
  char fmt[16];
  if (len == 0) sprintf(fmt, "%%s %%s");
  else sprintf(fmt, "%% %ds", len);
  printf(fmt, str);
}

void PrintNumber(Value value, u32 len)
{
  char fmt[16];
  if (len == 0) {
    sprintf(fmt, "%%f");
  } else {
    sprintf(fmt, "%% %d.1f", len);
  }

  printf(fmt, AsNum(value));
}

void PrintIndex(Value value, u32 len)
{
  PrintInt(AsIndex(value), len);
}

void PrintSymbol(Value value, u32 len)
{
  u32 strlen = StringLength(SymbolName(value), 0, LongestSymLength());
  u32 padding = (strlen > len) ? 0 : len - strlen;
  for (u32 i = 0; i < padding; i++) printf(" ");
  printf("%s", SymbolName(value));
}

void PrintPair(Value value, u32 len)
{
  PrintInt(AsPair(value), len);
}

void PrintVector(Value value, u32 len)
{
  PrintInt(AsVec(value), len);
}

void PrintValue(Value value, u32 len)
{
  printf("%s ", TypeAbbr(value));

  switch (TypeOf(value)) {
  case NUMBER:  PrintNumber(value, len); break;
  case INDEX:   PrintIndex(value, len); break;
  case SYMBOL:  PrintSymbol(value, len); break;
  case PAIR:    PrintPair(value, len); break;
  case VECTOR:  PrintVector(value, len); break;
  // case TRUE:    PrintStr(value, "", len); break;
  // case FALSE:   PrintStr(value, "", len); break;
  // case NIL:     PrintStr(value, "", len); break;
  default: for (u32 i = 0; i < len; i++) printf(" ");
  }
}

char *TypeAbbr(Value value)
{
  switch (TypeOf(value)) {
  case NUMBER:  return "#";
  case INDEX:   return "№";
  case SYMBOL:  return "★";
  case PAIR:    return "⚭";
  case VECTOR:  return "⁞";
  case TRUE:    return "⊤";
  case FALSE:   return "⊥";
  case NIL:     return "∅";
  default:      return "⍰";
  }
}
