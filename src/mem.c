#include "mem.h"
#include "env.h"
#include "print.h"

#define NextFree(mem)   VecCount(mem->values)

void InitMem(Mem *mem, u32 size)
{
  if (size > 0) {
    mem->values = NewVec(Val, size);
  } else {
    mem->values = NULL;
  }
  VecPush(mem->values, nil);
  VecPush(mem->values, nil);
  InitStringTable(&mem->symbols);
}

void DestroyMem(Mem *mem)
{
  FreeVec(mem->values);
  DestroyStringTable(&mem->symbols);
}

Val MakePair(Mem *mem, Val head, Val tail)
{
  Val pair = PairVal(NextFree(mem));

  VecPush(mem->values, head);
  VecPush(mem->values, tail);

  return pair;
}

Val Head(Mem *mem, Val pair)
{
  if (IsNil(pair)) return nil;
  u32 index = RawVal(pair);
  return mem->values[index];
}

Val Tail(Mem *mem, Val pair)
{
  if (IsNil(pair)) return nil;
  u32 index = RawVal(pair);
  return mem->values[index+1];
}

void SetHead(Mem *mem, Val pair, Val val)
{
  Assert(!IsNil(pair));
  u32 index = RawVal(pair);
  mem->values[index] = val;
}

void SetTail(Mem *mem, Val pair, Val val)
{
  Assert(!IsNil(pair));
  u32 index = RawVal(pair);
  mem->values[index+1] = val;
}

u32 ListLength(Mem *mem, Val list)
{
  u32 length = 0;
  while (!IsNil(list)) {
    length++;
    list = Tail(mem, list);
  }
  return length;
}

Val ListAt(Mem *mem, Val list, u32 index)
{
  if (IsNil(list)) return nil;
  if (index == 0) return Head(mem, list);
  return ListAt(mem, Tail(mem, list), index - 1);
}

Val ListFrom(Mem *mem, Val list, u32 index)
{
  if (IsNil(list)) return nil;
  if (index == 0) return list;
  return ListFrom(mem, Tail(mem, list), index - 1);
}

Val ReverseOnto(Mem *mem, Val list, Val tail)
{
  if (IsNil(list)) return tail;
  return ReverseOnto(mem, Tail(mem, list), MakePair(mem, Head(mem, list), tail));
}

Val ListAppend(Mem *mem, Val list, Val value)
{
  list = ReverseOnto(mem, list, nil);
  return ReverseOnto(mem, MakePair(mem, value, list), nil);
}

Val ListConcat(Mem *mem, Val list1, Val list2)
{
  list1 = ReverseOnto(mem, list1, nil);
  return ReverseOnto(mem, list1, list2);
}

bool IsTagged(Mem *mem, Val list, char *tag)
{
  if (!IsPair(list)) return false;
  return Eq(Head(mem, list), SymbolFor(tag));
}

bool IsTuple(Mem *mem, Val tuple)
{
  if (!IsObj(tuple)) return false;
  u32 index = RawVal(tuple);
  return IsTupleHeader(mem->values[index]);
}

Val MakeTuple(Mem *mem, u32 count)
{
  Val tuple = ObjVal(NextFree(mem));
  VecPush(mem->values, TupleHeader(count));

  if (count == 0) {
    VecPush(mem->values, nil);
  }

  for (u32 i = 0; i < count; i++) {
    VecPush(mem->values, nil);
  }

  return tuple;
}

u32 TupleLength(Mem *mem, Val tuple)
{
  Assert(IsTuple(mem, tuple));
  u32 index = RawVal(tuple);
  return RawVal(mem->values[index]);
}

Val TupleAt(Mem *mem, Val tuple, u32 i)
{
  Assert(IsTuple(mem, tuple));
  u32 index = RawVal(tuple);
  return mem->values[index + i + 1];
}

void TupleSet(Mem *mem, Val tuple, u32 i, Val val)
{
  Assert(IsTuple(mem, tuple));
  u32 index = RawVal(tuple);
  mem->values[index + i + 1] = val;
}

bool IsBinary(Mem *mem, Val binary)
{
  if (!IsObj(binary)) return false;
  u32 index = RawVal(binary);
  return IsBinaryHeader(mem->values[index]);
}

u32 NumBinaryCells(u32 length)
{
  if (length == 0) return 1;
  return (length - 1) / sizeof(Val) + 1;
}

Val MakeBinaryFrom(Mem *mem, char *str, u32 length)
{
  Val binary = ObjVal(NextFree(mem));
  VecPush(mem->values, BinaryHeader(length));

  if (length == 0) {
    VecPush(mem->values, (Val)0);
    return binary;
  }

  u32 num_words = NumBinaryCells(length);
  GrowVec(mem->values, num_words);

  u8 *bytes = BinaryData(mem, binary);
  for (u32 i = 0; i < length; i++) {
    bytes[i] = str[i];
  }

  return binary;
}

Val MakeBinary(Mem *mem, u32 length)
{
  Val binary = ObjVal(NextFree(mem));
  VecPush(mem->values, BinaryHeader(length));
  if (length == 0) {
    VecPush(mem->values, (Val)0);
    return binary;
  }

  u32 num_words = NumBinaryCells(length);
  GrowVec(mem->values, num_words);

  u8 *bytes = BinaryData(mem, binary);
  for (u32 i = 0; i < length; i++) {
    bytes[i] = 0;
  }

  return binary;
}

u32 BinaryLength(Mem *mem, Val binary)
{
  Assert(IsBinary(mem, binary));
  u32 index = RawVal(binary);
  return RawVal(mem->values[index]);
}

u8 *BinaryData(Mem *mem, Val binary)
{
  Assert(IsBinary(mem, binary));
  u32 index = RawVal(binary);
  return (u8*)(mem->values + index + 1);
}

void BinaryToString(Mem *mem, Val binary, char *dst)
{
  Assert(IsBinary(mem, binary));

  u32 length = BinaryLength(mem, binary);
  for (u32 i = 0; i < length; i++) {
    dst[i] = BinaryData(mem, binary)[i];
  }
  dst[length] = '\0';
}

static u32 IOListToString(Mem *mem, Val list, Buf *buf)
{
  u32 len = 0;
  while (!IsNil(list)) {
    Val val = Head(mem, list);
    if (IsInt(val)) {
      u8 c = RawInt(val);
      len += AppendByte(buf, c);
    } else if (IsSym(val)) {
      len += Append(buf, (u8*)SymbolName(mem, val), SymbolLength(mem, val));
    } else if (IsBinary(mem, val)) {
      len += Append(buf, BinaryData(mem, val), BinaryLength(mem, val));
    } else if (IsList(mem, val)) {
      if (!IOListToString(mem, val, buf)) {
        return len;
      }
    } else {
      return len;
    }
    list = Tail(mem, list);
  }
  return len;
}

u32 ValToString(Mem *mem, Val val, Buf *buf)
{
  if (IsNil(val)) {
    return Append(buf, (u8*)"nil", 3);
  } else if (IsNum(val)) {
    return AppendFloat(buf, RawNum(val), 4);
  } else if (IsInt(val)) {
    return AppendInt(buf, RawInt(val));
  } else if (IsSym(val)) {
    char *str = SymbolName(mem, val);
    return Append(buf, (u8*)str, SymbolLength(mem, val));
  } else if (IsBinary(mem, val)) {
    return Append(buf, BinaryData(mem, val), BinaryLength(mem, val));
  } else if (IsList(mem, val)) {
    return IOListToString(mem, val, buf);
  } else {
    return 0;
  }
}

u32 PrintValStr(Mem *mem, Val val)
{
  return ValToString(mem, val, output);
}

u32 ValStrLen(Mem *mem, Val val)
{
  if (IsSym(val)) {
    return SymbolLength(mem, val);
  } else if (IsBinary(mem, val)) {
    return BinaryLength(mem, val);
  } else {
    char str[255];
    Buf buf = MemBuf(str, 255);
    ValToString(mem, val, &buf);
    return buf.count;
  }
}

void PrintMem(Mem *mem)
{
  u32 ncols = 8;
  u32 stride = VecCount(mem->values) / ncols + 1;

  // as each row is printed, these store how many cells are left in an object, for each col
  u32 bins[ncols];
  u32 objs[ncols];
  for (u32 c = 0; c < ncols; c++) {
    bins[c] = 0;
    objs[c] = 0;
  }

  // since each col might start in the middle of an object, first scan through mem to mark the
  // starting values for the bins/objs arrays
  u32 i = 0;
  for (u32 c = 0; c < ncols-1 && i < VecCount(mem->values); c++) {
    u32 next_col = (c+1)*stride;
    while (i < next_col && i < VecCount(mem->values)) {
      u32 len = 2;
      if (IsTupleHeader(mem->values[i])) len = Max(1, RawVal(mem->values[i])) + 1;
      if (IsBinaryHeader(mem->values[i])) len = NumBinaryCells(RawVal(mem->values[i])) + 1;

      // if the length would go over a column, we initialize the column's byte/obj count
      if (i + len > next_col) {
        u32 left = (i+len) - next_col;
        if (IsBinaryHeader(mem->values[i])) {
          bins[c+1] = left;
        } else {
          objs[c+1] = left;
        }
      }
      i += len;
    }
  }

  // print header
  u32 actual_cols = Min(VecCount(mem->values), ncols);

  for (u32 c = 0; c < actual_cols; c++) {
    if (c == 0) Print("┏");
    else Print("┳");
    Print("━━━━┯━━━━━━━━━━");
  }
  Print("┓\n");

  for (u32 i = 0; i < stride; i++) {
    for (u32 c = 0; c < ncols; c++) {
      u32 n = i+c*stride;
      if (n < VecCount(mem->values)) {
        Print("┃");
        PrintIntN(n, 4, ' ');
        Print("│");


        Val value = mem->values[n];
        if (bins[c] > 0) {
          bins[c]--;

          char *data = (char*)(&value.as_v);
          if (IsPrintable(data[0]) && IsPrintable(data[1]) && IsPrintable(data[2]) && IsPrintable(data[3])) {
            Print("    \"");
            for (u32 ch = 0; ch < 4; ch++) PrintChar(data[ch]);
            Print("\"");
          } else {
            Print("  ");
            PrintHexN(value.as_v, 8, '0');
          }
        } else if (objs[c] > 0) {
          objs[c]--;

          PrintRawValN(mem->values[n], 10, mem);
        } else if (IsBinaryHeader(value)) {
          bins[c] = NumBinaryCells(RawVal(value));
          PrintRawValN(mem->values[n], 10, mem);
        } else if (IsTupleHeader(value)) {
          objs[c] = Max(1, RawVal(value));
          PrintRawValN(mem->values[n], 10, mem);
        } else {
          objs[c] = 1;
          PrintRawValN(mem->values[n], 10, mem);
        }
      } else {
        Print("┃    │          ");
      }
    }
    Print("┃\n");
  }

  // print footer
  for (u32 c = 0; c < actual_cols; c++) {
    if (c == 0) Print("┗");
    else Print("┻");
    Print("━━━━┷━━━━━━━━━━");
  }
  Print("┛\n");
}
