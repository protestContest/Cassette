#include "string.h"

Val StringValid(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArityError;
    return nil;
  }
  Val str = TupleGet(args, 0, mem);
  if (!IsBinary(str, mem)) {
    vm->error = TypeError;
    return str;
  }

  char *data = BinaryData(str, mem);
  for (u32 i = 0; i < BinaryLength(str, mem); i++) {
    if ((data[i] & 0xF8) == 0xF0) {
      if (i + 4 > BinaryLength(str, mem)) return SymbolFor("false");
      if ((data[i+1] & 0xC0) != 0x80) return SymbolFor("false");
      if ((data[i+2] & 0xC0) != 0x80) return SymbolFor("false");
      if ((data[i+3] & 0xC0) != 0x80) return SymbolFor("false");
      i += 3;
    } else if ((data[i] & 0xF0) == 0xE0) {
      if (i + 3 > BinaryLength(str, mem)) return SymbolFor("false");
      if ((data[i+1] & 0xC0) != 0x80) return SymbolFor("false");
      if ((data[i+2] & 0xC0) != 0x80) return SymbolFor("false");
      i += 2;
    } else if ((data[i] & 0xE0) == 0xC0) {
      if (i + 2 > BinaryLength(str, mem)) return SymbolFor("false");
      if ((data[i+1] & 0xC0) != 0x80) return SymbolFor("false");
      i++;
    } else if ((data[i] & 0x80) != 0x00) {
      return SymbolFor("false");
    }
  }
  return SymbolFor("true");
}

Val StringLength(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArityError;
    return nil;
  }
  Val str = TupleGet(args, 0, mem);
  if (!IsBinary(str, mem)) {
    vm->error = TypeError;
    return str;
  }

  char *data = BinaryData(str, mem);
  u32 count = 0;
  for (u32 i = 0; i < BinaryLength(str, mem);) {
    count++;
    i++;
    while (i < BinaryLength(str, mem) && (data[i] & 0xC0) == 0x80) i++;
  }
  return IntVal(count);
}

Val StringAt(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArityError;
    return nil;
  }
  Val str = TupleGet(args, 0, mem);
  if (!IsBinary(str, mem)) {
    vm->error = TypeError;
    return str;
  }
  Val index = TupleGet(args, 1, mem);
  if (!IsInt(index)) {
    vm->error = TypeError;
    return index;
  }

  char *data = BinaryData(str, mem);
  u32 count = 0;
  u32 start = 0;
  while (count < RawInt(index) && start < BinaryLength(str, mem)) {
    count++;
    start++;
    while (start < BinaryLength(str, mem) && (data[start] & 0xC0) == 0x80) start++;
  }
  u32 end = start + 1;
  while (end < BinaryLength(str, mem) && (data[end] & 0xC0) == 0x80) end++;

  return SliceBinary(str, start, end, mem);
}

Val StringSlice(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  if (TupleLength(args, mem) != 3) {
    vm->error = ArityError;
    return nil;
  }
  Val str = TupleGet(args, 0, mem);
  if (!IsBinary(str, mem)) {
    vm->error = TypeError;
    return str;
  }
  Val a = TupleGet(args, 1, mem);
  if (!IsInt(a)) {
    vm->error = TypeError;
    return a;
  }
  Val b = TupleGet(args, 1, mem);
  if (!IsInt(b)) {
    vm->error = TypeError;
    return b;
  }

  char *data = BinaryData(str, mem);
  u32 count = 0;
  u32 start = 0;
  while (count < RawInt(a) && start < BinaryLength(str, mem)) {
    count++;
    start++;
    while (start < BinaryLength(str, mem) && (data[start] & 0xC0) == 0x80) start++;
  }
  u32 end = start;
  while (count < RawInt(b) && end < BinaryLength(str, mem)) {
    count++;
    end++;
    while (end < BinaryLength(str, mem) && (data[end] & 0xC0) == 0x80) end++;
  }

  return SliceBinary(str, start, end, mem);
}

