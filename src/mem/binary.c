#include "mem.h"
#include "univ/system.h"
#include "univ/str.h"

Val MakeBinary(u32 size, Mem *mem)
{
  Val binary;
  u32 cells = (size == 0) ? 1 : NumBinCells(size);
  u32 i;

  if (mem->count + cells + 1 > mem->capacity) CollectGarbage(mem);
  if (mem->count + cells + 1 > mem->capacity) ResizeMem(mem, 2*mem->capacity);
  Assert(mem->count + cells + 1 <= mem->capacity);

  binary = ObjVal(mem->count);
  PushMem(mem, BinaryHeader(size));
  for (i = 0; i < cells; i++) PushMem(mem, 0);
  return binary;
}

Val BinaryFrom(char *str, u32 size, Mem *mem)
{
  Val bin = MakeBinary(size, mem);
  char *data = BinaryData(bin, mem);
  Copy(str, data, size);
  return bin;
}

bool BinaryContains(Val binary, Val item, Mem *mem)
{
  char *bin_data = BinaryData(binary, mem);
  u32 bin_len = BinaryLength(binary, mem);

  if (IsInt(item)) {
    u32 i;
    u8 byte;
    if (RawInt(item) < 0 || RawInt(item) > 255) return false;
    byte = RawInt(item);
    for (i = 0; i < bin_len; i++) {
      if (bin_data[i] == byte) return true;
    }
  } else if (IsBinary(item, mem)) {
    char *item_data = BinaryData(item, mem);
    u32 item_len = BinaryLength(item, mem);
    i32 index = FindString(item_data, item_len, bin_data, bin_len);
    return index >= 0;
  }

  return false;
}

Val BinaryCat(Val binary1, Val binary2, Mem *mem)
{
  Val result;
  u32 len1, len2;
  char *data;
  len1 = BinaryLength(binary1, mem);
  len2 = BinaryLength(binary2, mem);
  result = MakeBinary(len1+len2, mem);
  data = BinaryData(result, mem);
  Copy(BinaryData(binary1, mem), data, len1);
  Copy(BinaryData(binary2, mem), data+len1, len2);
  return result;
}
