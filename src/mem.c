#include "mem.h"
#include <univ/vec.h>
#include <univ/symbol.h>
#include <univ/math.h>
#include <univ/str.h>

static val *mem = 0;

void InitMem(u32 size)
{
  DestroyMem();
  mem = NewVec(val, size);
  VecPush(mem, 0);
  VecPush(mem, 0);
  SetSymbolSize(valBits);
}

void DestroyMem(void)
{
  if (mem) FreeVec(mem);
  mem = 0;
}

u32 MemSize(void)
{
  return VecCount(mem);
}

u32 MemFree(void)
{
  return VecCapacity(mem) - VecCount(mem);
}

static u32 MemAlloc(u32 count)
{
  u32 index;
  if (!mem) InitMem(256);
  index = VecCount(mem);
  GrowVec(mem, Max(2, count));
  return index;
}

#define MemGet(index)       (mem[index])
#define MemSet(index, val)  (mem[index] = val)

val CopyObj(val value, val *oldmem)
{
  u32 index, i, len;
  if (value == 0 || !(IsPair(value) || IsObj(value))) return value;
  index = RawVal(value);
  if (oldmem[index] == SymVal(Symbol("*moved*"))) {
    return oldmem[index+1];
  }
  if (IsObj(value)) {
    if (IsBinHdr(oldmem[index])) {
      len = RawVal(oldmem[index]);
      value = NewBinary(len);
      Copy(oldmem+index+1, BinaryData(value), len);
    } else if (IsTupleHdr(oldmem[index])) {
      len = RawVal(oldmem[index]);
      value = Tuple(len);
      for (i = 0; i < len; i++) {
        TupleSet(value, i, oldmem[index+i+1]);
      }
    }
  } else {
    value = Pair(oldmem[index], oldmem[index+1]);
  }
  oldmem[index] = SymVal(Symbol("*moved*"));
  oldmem[index+1] = value;
  return value;
}

void CollectGarbage(val *roots)
{
  u32 i, scan;
  val *oldmem = mem;

  printf("GARBAGE DAY!!!\n");

  mem = NewVec(val, VecCapacity(mem));
  VecPush(mem, 0);
  VecPush(mem, 0);

  for (i = 0; i < VecCount(roots); i++) {
    roots[i] = CopyObj(roots[i], oldmem);
  }

  scan = 2;
  while (scan < MemSize()) {
    val next = MemGet(scan);
    if (IsBinHdr(next)) {
      scan += BinSpace(RawVal(next)) + 1;
    } else if (IsTupleHdr(next)) {
      for (i = 0; i < RawVal(next); i++) {
        MemSet(scan+1+i, CopyObj(MemGet(scan+1+i), oldmem));
      }
      scan += RawVal(next) + 1;
    } else {
      MemSet(scan, CopyObj(MemGet(scan), oldmem));
      MemSet(scan+1, CopyObj(MemGet(scan+1), oldmem));
      scan += 2;
    }
  }

  FreeVec(oldmem);
}

val Pair(val head, val tail)
{
  i32 index = MemAlloc(2);
  MemSet(index, head);
  MemSet(index+1, tail);
  return PairVal(index);
}

val Head(val pair)
{
  return MemGet(RawVal(pair));
}

val Tail(val pair)
{
  return MemGet(RawVal(pair)+1);
}

void SetHead(val pair, val head)
{
  i32 index = RawVal(pair);
  MemSet(index, head);
}

void SetTail(val pair, val tail)
{
  i32 index = RawVal(pair);
  MemSet(index+1, tail);
}

u32 ListLength(val list)
{
  u32 count = 0;
  while (list && ValType(list) == pairType) {
    count++;
    list = Tail(list);
  }
  return count;
}

val ListGet(val list, u32 index)
{
  u32 i;
  for (i = 0; i < index; i++) list = Tail(list);
  return Head(list);
}

val ReverseList(val list, val tail)
{
  while (list && ValType(list) == pairType) {
    tail = Pair(Head(list), tail);
    list = Tail(list);
  }
  return tail;
}

val ListJoin(val left, val right)
{
  left = ReverseList(left, 0);
  return ReverseList(left, right);
}

val ListTrunc(val list, u32 index)
{
  u32 i;
  val truncated = 0;
  for (i = 0; i < index; i++) {
    truncated = Pair(Head(list), truncated);
    list = Tail(list);
  }
  return ReverseList(truncated, 0);
}

val ListSkip(val list, u32 index)
{
  u32 i;
  for (i = 0; i < index; i++) list = Tail(list);
  return list;
}

bool InList(val item, val list)
{
  while (list) {
    if (item == Head(list)) return true;
    list = Tail(list);
  }
  return false;
}

val Tuple(u32 length)
{
  u32 index = MemAlloc(length+1);
  MemSet(index, TupleHeader(length));
  return ObjVal(index);
}

u32 TupleLength(val tuple)
{
  return RawVal(MemGet(RawVal(tuple)));
}

val TupleGet(val tuple, u32 index)
{
  if (index < 0 || index >= TupleLength(tuple)) return 0;
  return MemGet(RawVal(tuple)+index+1);
}

void TupleSet(val tuple, u32 index, val value)
{
  if (index < 0 || index >= TupleLength(tuple)) return;
  MemSet(RawVal(tuple)+index+1, value);
}

val TupleJoin(val left, val right)
{
  u32 i;
  val tuple = Tuple(TupleLength(left) + TupleLength(right));
  for (i = 0; i < TupleLength(left); i++) {
    TupleSet(tuple, i, TupleGet(left, i));
  }
  for (i = 0; i < TupleLength(right); i++) {
    TupleSet(tuple, i+TupleLength(left), TupleGet(right, i));
  }
  return tuple;
}

val TupleSlice(val tuple, u32 start, u32 end)
{
  u32 i;
  u32 len = (end > start) ? end - start : 0;
  val slice = Tuple(Min(len, TupleLength(tuple)));
  for (i = 0; i < TupleLength(slice); i++) {
    TupleSet(slice, i, TupleGet(tuple, i + start));
  }
  return slice;
}

val NewBinary(u32 length)
{
  u32 index = MemAlloc(BinSpace(length) + 1);
  MemSet(index, BinHeader(length));
  return ObjVal(index);
}

val Binary(char *str)
{
  return BinaryFrom(str, strlen(str));
}

val BinaryFrom(char *str, u32 length)
{
  val bin = NewBinary(length);
  char *binData = BinaryData(bin);
  Copy(str, binData, length);
  return bin;
}

u32 BinaryLength(val bin)
{
  return RawVal(MemGet(RawVal(bin)));
}

char *BinaryData(val bin)
{
  return (char*)(mem + RawVal(bin) + 1);
}

val BinaryGet(val bin, u32 index)
{
  if (index < 0 || index >= BinaryLength(bin)) return 0;
  return BinaryData(bin)[index];
}

void BinarySet(val bin, u32 index, val value)
{
  if (index < 0 || index >= BinaryLength(bin)) return;
  BinaryData(bin)[index] = value;
}

val BinaryJoin(val left, val right)
{
  val bin = NewBinary(BinaryLength(left) + BinaryLength(right));
  Copy(BinaryData(left), BinaryData(bin), BinaryLength(left));
  Copy(BinaryData(right),
       BinaryData(bin) + BinaryLength(left),
       BinaryLength(right));
  return bin;
}

val BinarySlice(val bin, u32 start, u32 end)
{
  u32 len = (end > start) ? end - start : 0;
  val slice = NewBinary(Min(len, TupleLength(bin)));
  Copy(BinaryData(bin)+start, BinaryData(slice), BinaryLength(slice));
  return slice;
}

bool BinIsPrintable(val bin)
{
  u32 i;
  for (i = 0; i < BinaryLength(bin); i++) {
    if (!IsPrintable(BinaryGet(bin, i))) return false;
  }
  return true;
}

bool ValEq(val a, val b)
{
  if (!a && !b) {
    return true;
  } else if (IsPair(a) && IsPair(b)) {
    return ValEq(Head(a), Head(b)) && ValEq(Tail(a), Tail(b));
  } else if (IsTuple(a) && IsTuple(b)) {
    u32 i;
    if (TupleLength(a) != TupleLength(b)) return false;
    for (i = 0; i < TupleLength(a); i++) {
      if (!ValEq(TupleGet(a, i), TupleGet(b, i))) return false;
    }
    return true;
  } else if (IsBinary(a) && IsBinary(b)) {
    char *adata = BinaryData(a);
    char *bdata = BinaryData(b);
    u32 i;
    if (BinaryLength(a) != BinaryLength(b)) return false;
    for (i = 0; i < BinaryLength(a); i++) {
      if (adata[i] != bdata[i]) return false;
    }
    return true;
  } else if (IsInt(a) && IsInt(b)) {
    return a == b;
  } else if (IsSym(a) && IsSym(b)) {
    return a == b;
  } else {
    return false;
  }
}

u32 FormatValInto(val value, val bin, u32 index)
{
  char *str = bin ? BinaryData(bin) + index : 0;

  if (IsInt(value)) return WriteNum(RawInt(value), str);
  if (IsBinary(value)) {
    u32 len = WriteStr(BinaryData(value), BinaryLength(value), str);
    return len;
  }
  if (IsPair(value) && value) {
    u32 head_len = FormatValInto(Head(value), bin, index);
    u32 tail_len = FormatValInto(Tail(value), bin, index + head_len);
    return head_len + tail_len;
  }
  return 0;
}

val FormatVal(val value)
{
  u32 len = FormatValInto(value, 0, 0) + 1;
  val bin = NewBinary(len);
  FormatValInto(value, bin, 0);
  return bin;
}

u32 InspectValInto(val value, val bin, u32 index)
{
  char *str = bin ? BinaryData(bin) + index : 0;

  if (value == 0) return WriteStr("nil", 3, str);
  if (IsInt(value)) return WriteNum(RawInt(value), str);
  if (IsBinary(value)) {
    char *data = BinaryData(value);
    u32 len = BinaryLength(value);
    return WriteStr("\"", 1, str) +
           WriteStr(data, len, str ? str + 1 : 0) +
           WriteStr("\"", 1, str ? str + 1 + len : 0);
  }
  if (IsSym(value)) {
    char *data = SymbolName(RawVal(value));
    u32 len = strlen(data);
    return WriteStr(":", 1, str) +
           WriteStr(data, len, str ? str + 1 : 0);
  }
  if (IsTuple(value)) {
    u32 i;
    u32 len = WriteStr("{", 1, str);
    for (i = 0; i < TupleLength(value); i++) {
      len += InspectValInto(TupleGet(value, i), bin, index + len);
      if (i < TupleLength(value) - 1) {
        len += WriteStr(", ", 2, str ? str + len : 0);
      }
    }
    len += WriteStr("}", 1, str ? str + len : 0);
    return len;
  }
  if (IsPair(value)) {
    u32 len = WriteStr("[", 1, str);
    while (value) {
      len += InspectValInto(Head(value), bin, index + len);
      value = Tail(value);
      if (!IsPair(value)) {
        len += WriteStr(" | ", 3, str ? str + len : 0);
        len += InspectValInto(value, bin, index + len);
        break;
      }
      if (value) len += WriteStr(", ", 2, str ? str + len : 0);
    }
    len += WriteStr("]", 1, str ? str + len : 0);
    return len;
  }
  return 0;
}

val InspectVal(val value)
{
  u32 len = InspectValInto(value, 0, 0);
  val str = NewBinary(len);
  InspectValInto(value, str, 0);
  return str;
}

char *MemValStr(val value)
{
  char *str;
  if (value == 0) {
    str = malloc(4);
    str[0] = 'n';
    str[1] = 'i';
    str[2] = 'l';
    str[3] = 0;
    return str;
  }
  if (IsInt(value)) {
    str = malloc(NumDigits(value, 10) + 1);
    WriteNum(RawInt(value), str);
    return str;
  }
  if (IsSym(value)) {
    char *data = SymbolName(RawVal(value));
    str = malloc(strlen(data) + 2);
    str[0] = ':';
    WriteStr(data, strlen(data)+1, str+1);
    return str;
  }
  if (IsBinary(value) && BinaryLength(value) < 8 && BinIsPrintable(value)) {
    u32 len = BinaryLength(value);
    char *data = BinaryData(value);
    str = malloc(len + 3);
    str[0] = '"';
    WriteStr(data, len+1, str+1);
    str[len + 1] = '"';
    str[len + 2] = 0;
    return str;
  }

  str = malloc(NumDigits(RawVal(value), 10) + 2);
  if (IsBinary(value)) {
    str[0] = 'b';
  } else if (IsTuple(value)) {
    str[0] = 't';
  } else if (IsPair(value)) {
    str[0] = 'p';
  } else if (IsTupleHdr(value)) {
    str[0] = '#';
  } else if (IsBinHdr(value)) {
    str[0] = '$';
  } else {
    str[0] = '?';
  }
  WriteNum(RawVal(value), str + 1);
  str[NumDigits(RawVal(value), 10) + 1] = 0;
  return str;
}

void DumpMem(void)
{
  u32 i;
  u32 numCols = 6;
  u32 colWidth = 10;

  printf("%*d|", colWidth, 0);
  for (i = 0; i < VecCount(mem); i++) {
    char *str = MemValStr(MemGet(i));
    printf("%*s|", colWidth, str);
    free(str);

    if (i % numCols == numCols - 1) {
      printf("\n");
      printf("%*d|", colWidth, i);
    }
  }
  printf("\n");
  printf("%d / %d\n", VecCount(mem), VecCapacity(mem));
}
