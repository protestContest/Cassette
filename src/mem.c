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

val BinaryFrom(char *data, u32 length)
{
  val bin = NewBinary(length);
  char *binData = BinaryData(bin);
  Copy(data, binData, length);
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
  Copy(BinaryData(right), BinaryData(bin)+BinaryLength(left), BinaryLength(right));
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

char *ValStr(val value, char *str)
{
  i32 len;

  if (value == 0) {
    if (!str) str = malloc(4);
    strcpy(str, "nil");
    str[3] = 0;
    return str;
  } else if (IsPair(value)) {
    len = NumDigits(RawVal(value), 10) + 2;
    if (!str) str = malloc(len);
    str[0] = 'p';
    snprintf(str+1, len-1, "%d", RawVal(value));
    return str;
  } else if (IsTuple(value)) {
    len = NumDigits(RawVal(value), 10) + 2;
    if (!str) str = malloc(len);
    str[0] = 't';
    snprintf(str+1, len-1, "%d", RawVal(value));
    return str;
  } else if (IsBinary(value)) {
    len = BinaryLength(value);
    if (len <= 8 && BinIsPrintable(value)) {
      if (!str) str = malloc(len+3);
      str[0] = '"';
      Copy(BinaryData(value), str+1, BinaryLength(value));
      str[len+1] = '"';
      str[len+2] = 0;
    } else {
      len = NumDigits(RawVal(value), 10) + 2;
      if (!str) str = malloc(len);
      str[0] = 'b';
      snprintf(str+1, len-1, "%d", RawVal(value));
    }
    return str;
  } else if (IsInt(value)) {
    len = NumDigits(RawInt(value), 10) + 1;
    if (!str) str = malloc(len);
    snprintf(str, len, "%d", RawInt(value));
    return str;
  } else if (IsSym(value)) {
    char *name = SymbolName(RawVal(value));
    len = strlen(name) + 1;
    if (!str) str = malloc(len+1);
    str[0] = ':';
    Copy(name, str+1, len-1);
    str[len] = 0;
    return str;
  } else if (IsTupleHdr(value)) {
    len = NumDigits(RawVal(value), 10) + 2;
    if (!str) str = malloc(len);
    str[0] = '#';
    snprintf(str+1, len-1, "%d", RawVal(value));
    return str;
  } else if (IsBinHdr(value)) {
    len = NumDigits(RawVal(value), 10) + 2;
    if (!str) str = malloc(len);
    str[0] = '$';
    snprintf(str+1, len-1, "%d", RawVal(value));
    return str;
  } else {
    if (!str) str = malloc(9);
    snprintf(str, 9, "%08X", value);
    return str;
  }
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

u32 ValStrLen(val value)
{
  if (IsInt(value)) return NumDigits(RawInt(value), 10);
  if (IsBinary(value)) return BinaryLength(value);
  if (IsPair(value) && value) {
    return ValStrLen(Head(value)) + ValStrLen(Tail(value));
  }
  return 0;
}

char *FormatVal(val value, char *buf)
{
  if (!buf) {
    u32 len = ValStrLen(value);
    buf = malloc(len+1);
    buf[len] = 0;
  }
  if (IsInt(value)) return ValStr(value, buf);
  if (IsBinary(value)) {
    Copy(BinaryData(value), buf, BinaryLength(value));
  }
  if (IsPair(value) && value) {
    u32 head_len = ValStrLen(Head(value));
    FormatVal(Head(value), buf);
    FormatVal(Tail(value), buf+head_len);
  }
  return buf;
}

void DumpMem(void)
{
  u32 i;
  u32 numCols = 6;
  u32 colWidth = 10;

  printf("%*d|", colWidth, 0);
  for (i = 0; i < VecCount(mem); i++) {
    char *str = ValStr(MemGet(i), 0);
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

