#include "binary.h"

Val MakeBinary(u32 length, Mem *mem)
{
  u32 index = MemSize(mem);
  u32 num_cells = NumBinaryCells(length);
  u32 i;
  PushVal(mem, BinaryHeader(length));
  for (i = 0; i < num_cells; i++) PushVal(mem, 0);
  return ObjVal(index);
}

Val BinaryFrom(void *data, u32 length, Mem *mem)
{
  Val binary = MakeBinary(length, mem);
  Copy(data, BinaryData(binary, mem), length);
  return binary;
}

bool IsBinary(Val binary, Mem *mem)
{
  return IsObj(binary) && IsBinaryHeader((*mem->values)[RawVal(binary)]);
}

u32 BinaryLength(Val binary, Mem *mem)
{
  return RawVal((*mem->values)[RawVal(binary)]);
}

void *BinaryData(Val binary, Mem *mem)
{
  return (*mem->values) + RawVal(binary) + 1;
}
