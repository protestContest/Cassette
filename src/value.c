#include "value.h"
#include "string.h"
#include "vm.h"
#include "printer.h"
#include <stdio.h>

ValType TypeOf(Value value)
{
  if (IsNumber(value))  return NUMBER;
  if (IsSymbol(value))  return SYMBOL;
  if (IsPair(value))    return PAIR;
  if (IsIndex(value))   return INDEX;
  if (IsObject(value))  return OBJECT;

  Error("Value not implemented: 0x%0X", value);
}

ObjType ObjTypeOf(Value value)
{
  if (IsBinary(value))    return BINARY;
  if (IsFunction(value))  return FUNCTION;
  if (IsTuple(value))     return TUPLE;
  if (IsDict(value))      return DICT;

  Error("Object type not implemented: 0x%0X", *HeapRef(value));
}

u32 ObjectSize(Value value)
{
  ObjHeader *obj = HeapRef(value);
  return HeaderValue(*obj);
}

Value MakeBinary(char *src, u32 start, u32 end)
{
  u32 size = (start >= end) ? 0 : end - start;
  Value obj = Allocate(BinaryHeader(size));
  char *str = BinaryData(obj);
  memcpy(str, src + start, size);
  return obj;
}

Value CreateBinary(char *src)
{
  u32 size = strlen(src);
  Value obj = Allocate(BinaryHeader(size));
  memcpy(BinaryData(obj), src, size);
  return obj;
}

Value CopySlice(Value text, Value start, Value end)
{
  u32 size = RawVal(end) - RawVal(start);
  Value obj = Allocate(BinaryHeader(size));
  char *src = BinaryData(text) + RawVal(start);
  char *dst = BinaryData(obj);
  memcpy(dst, src, size);
  return obj;
}
