#include "binary.h"

Val BinToList(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArgError;
    return nil;
  }
  Val bin = TupleGet(args, 0, mem);
  if (!IsBinary(bin, mem)) {
    vm->error = TypeError;
    return bin;
  }

  Val list = nil;
  for (u32 i = 0; i < BinaryLength(bin, mem); i++) {
    Val byte = IntVal(BinaryGetByte(bin, i, mem));
    list = Pair(byte, list, mem);
  }

  return ReverseList(list, mem);
}

Val BinTrunc(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 2) {
    vm->error = ArgError;
    return nil;
  }
  Val bin = TupleGet(args, 0, mem);
  if (!IsBinary(bin, mem)) {
    vm->error = TypeError;
    return bin;
  }
  Val index = TupleGet(args, 1, mem);
  if (!IsInt(index)) {
    vm->error = TypeError;
    return index;
  }

  return SliceBinary(bin, 0, RawInt(index), mem);
}

Val BinAfter(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 2) {
    vm->error = ArgError;
    return nil;
  }
  Val bin = TupleGet(args, 0, mem);
  if (!IsBinary(bin, mem)) {
    vm->error = TypeError;
    return bin;
  }
  Val index = TupleGet(args, 1, mem);
  if (!IsInt(index)) {
    vm->error = TypeError;
    return index;
  }

  return SliceBinary(bin, RawInt(index), BinaryLength(bin, mem), mem);
}

Val BinSlice(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 3) {
    vm->error = ArgError;
    return nil;
  }
  Val bin = TupleGet(args, 0, mem);
  if (!IsBinary(bin, mem)) {
    vm->error = TypeError;
    return bin;
  }
  Val start = TupleGet(args, 1, mem);
  if (!IsInt(start)) {
    vm->error = TypeError;
    return start;
  }
  Val end = TupleGet(args, 1, mem);
  if (!IsInt(end)) {
    vm->error = TypeError;
    return end;
  }

  return SliceBinary(bin, RawInt(start), RawInt(end), mem);
}

Val BinJoin(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 2) {
    vm->error = ArgError;
    return nil;
  }
  Val b1 = TupleGet(args, 0, mem);
  if (!IsBinary(b1, mem)) {
    vm->error = TypeError;
    return b1;
  }
  Val b2 = TupleGet(args, 1, mem);
  if (!IsBinary(b2, mem)) {
    vm->error = TypeError;
    return b2;
  }

  return JoinBinaries(b1, b2, mem);
}

