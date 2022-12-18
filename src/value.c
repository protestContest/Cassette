#include "value.h"
#include "string.h"
#include "vm.h"
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

  fprintf(stderr, "Object type not implemented: 0x%0X\n", *HeapRef(value));
  exit(1);
}

u32 ObjectSize(Value value)
{
  ObjHeader *obj = HeapRef(value);
  return HeaderValue(*obj);
}

Value MakeBinary(char *src, u32 start, u32 end)
{
  u32 size = (start >= end) ? 0 : end - start;
  ObjHeader header = BinaryHeader(size);
  Value obj = Allocate(header, size);
  char *str = BinaryData(obj);
  memcpy(str, src + start, size);
  return obj;
}

Value MakeString(char *src)
{
  u32 size = strlen(src);
  return MakeBinary(src, 0, size);
}
