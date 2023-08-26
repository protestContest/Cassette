#include "io.h"

Val IOPrint(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  Val item = TupleGet(args, 0, mem);

  if (IsNum(item)) {
    Inspect(item, mem);
    Print("\n");
  } else if (IsSym(item)) {
    Print(SymbolName(item, mem));
    Print("\n");
  } else if (IsBinary(item, mem)) {
    PrintN(BinaryData(item, mem), BinaryLength(item, mem));
    Print("\n");
  } else {
    return SymbolFor("error");
  }

  return SymbolFor("ok");
}

Val IOInspect(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  Val item = TupleGet(args, 0, mem);
  Inspect(item, mem);
  Print("\n");
  return SymbolFor("ok");
}


static u32 IOListLength(Val iolist, Heap *mem)
{
  if (IsInt(iolist)) {
    return 1;
  } else if (IsPair(iolist)) {
    u32 len = 0;
    while (!IsNil(iolist)) {
      if (IsPair(iolist)) {
        len += IOListLength(Head(iolist, mem), mem);
        iolist = Tail(iolist, mem);
      } else {
        len += IOListLength(iolist, mem);
        iolist = nil;
      }
    }
    return len;
  } else if (IsBinary(iolist, mem)) {
    return BinaryLength(iolist, mem);
  } else {
    return 0;
  }
}

static u32 CopyIOListInto(char *data, Val iolist, Heap *mem)
{
  if (IsInt(iolist)) {
    data[0] = RawInt(iolist);
    return 1;
  } else if (IsPair(iolist)) {
    char *start = data;
    while (!IsNil(iolist)) {
      if (IsPair(iolist)) {
        data += CopyIOListInto(data, Head(iolist, mem), mem);
        iolist = Tail(iolist, mem);
      } else {
        data += CopyIOListInto(data, iolist, mem);
        iolist = nil;
      }
    }
    return data - start;
  } else if (IsBinary(iolist, mem)) {
    Copy(BinaryData(iolist, mem), data, BinaryLength(iolist, mem));
    return BinaryLength(iolist, mem);
  } else {
    return 0;
  }
}

Val IOListToBin(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  Val iolist = TupleGet(args, 0, mem);
  u32 length = IOListLength(iolist, mem);
  Val bin = MakeBinary(length, mem);
  CopyIOListInto(BinaryData(bin, mem), iolist, mem);
  return bin;
}

