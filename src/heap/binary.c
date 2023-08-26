#include "binary.h"
#include "list.h"

Val MakeBinary(u32 length, Heap *mem)
{
  u32 index = MemSize(mem);
  VecPush(mem->values, BinaryHeader(length));
  u32 num_cells = NumBinaryCells(length);
  for (u32 i = 0; i < num_cells; i++) {
    VecPush(mem->values, (Val){.as_i = 0});
  }
  return ObjVal(index);
}

Val BinaryFrom(void *data, u32 length, Heap *mem)
{
  Val binary = MakeBinary(length, mem);
  Copy(data, BinaryData(binary, mem), length);
  return binary;
}

bool IsBinary(Val binary, Heap *mem)
{
  return IsObj(binary) && IsBinaryHeader(mem->values[RawVal(binary)]);
}

u32 BinaryLength(Val binary, Heap *mem)
{
  return RawVal(mem->values[RawVal(binary)]);
}

void *BinaryData(Val binary, Heap *mem)
{
  return mem->values + RawVal(binary) + 1;
}

u8 BinaryGetByte(Val binary, u32 i, Heap *mem)
{
  return ((u8*)BinaryData(binary, mem))[i];
}

void BinarySetByte(Val binary, u32 i, u8 b, Heap *mem)
{
  ((u8*)BinaryData(binary, mem))[i] = b;
}

Val BinaryTrunc(Val binary, u32 index, Heap *mem)
{
  if (index == 0) return binary;
  u32 length = BinaryLength(binary, mem);
  if (index >= length) return binary;

  char *data = BinaryData(binary, mem);

  Val new_bin = MakeBinary(index, mem);
  char *new_data = BinaryData(new_bin, mem);
  Copy(data, new_data, index);
  return new_bin;
}

Val BinaryAfter(Val binary, u32 index, Heap *mem)
{
  if (index == 0) return binary;
  u32 length = BinaryLength(binary, mem);
  if (index >= length) return binary;

  char *data = BinaryData(binary, mem);

  Val new_bin = MakeBinary(length - index, mem);
  char *new_data = BinaryData(new_bin, mem);
  Copy(data + index, new_data, length - index);
  return new_bin;
}

Val JoinBinaries(Val b1, Val b2, Heap *mem)
{
  u32 a_length = BinaryLength(b1, mem);
  char *a_data = BinaryData(b1, mem);
  u32 b_length = BinaryLength(b2, mem);
  char *b_data = BinaryData(b2, mem);

  Val bin = MakeBinary(a_length + b_length, mem);
  char *data = BinaryData(bin, mem);

  Copy(a_data, data, a_length);
  Copy(b_data, data+a_length, b_length);
  return bin;
}
