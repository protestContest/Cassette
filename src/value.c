#include "value.h"
#include "symbol.h"
#include "string.h"
#include <stdio.h>

#define header_mask         0xE0000000
#define BinaryHeader(size)  ((size) & (~header_mask) | bin_mask)
#define HeaderValue(hdr)    ((hdr) & (~header_mask))

ValType TypeOf(Value value)
{
  if (IsNumber(value))  return NUMBER;
  if (IsSymbol(value))  return SYMBOL;
  if (IsPair(value))    return PAIR;
  if (IsIndex(value))   return INDEX;
  if (IsObject(value))  return OBJECT;

  fprintf(stderr, "Value not implemented: 0x%0X\n", value);
  exit(1);
}

ObjType ObjTypeOf(Value value)
{
  if (IsBinary(value))    return BINARY;
  if (IsFunction(value))  return FUNCTION;
  if (IsTuple(value))     return TUPLE;
  if (IsDict(value))      return DICT;

  fprintf(stderr, "Object type not implemented: 0x%0X\n", *ObjectRef(value));
  exit(1);
}

Value MakeBinary(char *src, u32 start, u32 end)
{
  u32 size = (start >= end) ? 0 : end - start;
  ObjHeader header = BinaryHeader(size);
  u32 index = AllocateObject(header, size);
  Value obj = ObjectVal(index);
  char *str = BinaryData(obj);
  memcpy(str, src + start, size);
  return obj;
}

u32 BinarySize(Value binary)
{
  ObjHeader *obj = ObjectRef(binary);
  return HeaderValue(*obj);
}

void PrintInt(u32 n, u32 len)
{
  char fmt[16];
  if (len == 0) sprintf(fmt, "%%u");
  else sprintf(fmt, "%% %du", len);
  printf(fmt, n);
}

void PrintNumber(Value value, u32 len)
{
  char fmt[16];
  if (len == 0) {
    sprintf(fmt, "%%f");
  } else {
    sprintf(fmt, "%% %d.1f", len);
  }

  printf(fmt, RawVal(value));
}

void PrintIndex(Value value, u32 len)
{
  PrintInt(RawVal(value), len);
}

void PrintBinary(Value value, u32 len)
{
  char *str = BinaryData(value);
  u32 size = BinarySize(value);
  u32 strlen = StringLength(str, 0, size);
  u32 padding = (strlen > len) ? 0 : len - strlen;

  for (u32 i = 0; i < padding; i++) printf(" ");
  ExplicitPrint(str, size);
}

void PrintSymbol(Value value, u32 len)
{
  PrintBinary(SymbolName(value), len);
}

void PrintPair(Value value, u32 len)
{
  PrintInt(RawVal(value), len);
}

void PrintFunction(Value value, u32 len)
{
  printf("0x%0X", value);
}

void PrintTuple(Value value, u32 len)
{
  printf("0x%0X", value);
}

void PrintDict(Value value, u32 len)
{
  printf("0x%0X", value);
}

void PrintObject(Value value, u32 len)
{
  switch(ObjTypeOf(value)) {
  case BINARY:    PrintBinary(value, len); break;
  case FUNCTION:  PrintFunction(value, len); break;
  case TUPLE:     PrintTuple(value, len); break;
  case DICT:      PrintDict(value, len); break;
  }
}

void PrintValue(Value value, u32 len)
{
  switch (TypeOf(value)) {
  case NUMBER:  PrintNumber(value, len); break;
  case INDEX:   PrintIndex(value, len); break;
  case SYMBOL:  PrintSymbol(value, len); break;
  case PAIR:    PrintPair(value, len); break;
  case OBJECT:  PrintObject(value, len); break;
  default: for (u32 i = 0; i < len; i++) printf(" ");
  }
}

char *TypeAbbr(Value value)
{
  switch (TypeOf(value)) {
  case NUMBER:  return "№";
  case INDEX:   return "#";
  case SYMBOL:  return "★";
  case PAIR:    return "⚭";
  case OBJECT: {
    switch (ObjTypeOf(value)) {
    case BINARY:    return "⨳";
    case FUNCTION:  return "λ";
    case TUPLE:     return "☰";
    case DICT:      return ":";
    }
  }
  default:      return "⍰";
  }
}
