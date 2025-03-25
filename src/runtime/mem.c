#include "runtime/mem.h"
#include "runtime/symbol.h"
#include "univ/hashmap.h"
#include "univ/math.h"
#include "univ/str.h"

#define MIN_CAPACITY  1000000

static Mem mem = {0};

#define Save(v, n)      mem.tmp[n] = (v)
#define Restore(v, n)   v = mem.tmp[n], mem.tmp[n] = 0

void InitMem(u32 size)
{
  size = Max(size, MIN_CAPACITY);
  mem.data = malloc(size*sizeof(u32));
  mem.capacity = size;
  mem.free = 2;
  mem.stack = size;
  mem.data[0] = 0;
  mem.data[1] = 0;
  mem.roots = 0;
  mem.num_roots = 0;
  mem.tmp[0] = 0;
  mem.tmp[1] = 0;
}

void DestroyMem(void)
{
  if (mem.data) free(mem.data);
  mem.capacity = 0;
  mem.data = 0;
  mem.free = 0;
  mem.stack = 0;
}

void SizeMem(u32 size)
{
  u32 stack_size = StackSize();
  u32 *stack = malloc(sizeof(u32)*stack_size);
  Copy(mem.data + mem.stack, stack, sizeof(u32)*stack_size);
  mem.data = realloc(mem.data, sizeof(u32)*size);
  mem.stack = size - stack_size;
  Copy(stack, mem.data + mem.stack, sizeof(u32)*stack_size);
  mem.capacity = size;
  free(stack);
}

u32 MemCapacity(void)
{
  return mem.capacity;
}

u32 MemFree(void)
{
  return mem.stack - mem.free;
}

static u32 MemAlloc(u32 count)
{
  u32 index;
  if (!mem.data) InitMem(MIN_CAPACITY);
  count = Max(2, count);

  if (MemFree() < count) CollectGarbage();
  if (MemFree() < count) {
    u32 needed = MemCapacity() + count - MemFree();
    SizeMem(Max(2*MemCapacity(), needed));
  }
  assert(MemFree() >= count);

  index = mem.free;
  mem.free += count;
  return index;
}

static u32 MemGet(u32 index)
{
  assert(index < mem.free);
  return mem.data[index];
}

static void MemSet(u32 index, u32 value)
{
  assert(index < mem.free);
  mem.data[index] = value;
}

void SetMemRoots(u32 *roots, u32 num_roots)
{
  mem.roots = roots;
  mem.num_roots = num_roots;
}

static u32 CopyObj(u32 value, u32 *oldmem, u32 *newmem, u32 *free)
{
  static u32 moved = 0;
  u32 index;

  if (!moved) moved = IntVal(Symbol("*moved*"));
  if (value == 0 || !IsObj(value)) return value;

  index = RawVal(value);

  if (oldmem[index] == moved) return oldmem[index+1];

  if (IsBinHdr(oldmem[index])) {
    u32 obj_index;
    u32 len = RawVal(oldmem[index]);
    obj_index = *free;
    *free += Max(1, BinSpace(len)) + 1;
    newmem[obj_index] = BinHeader(len);
    value = ObjVal(obj_index);
    Copy(oldmem+index+1, newmem+obj_index+1, len);
  } else if (IsTupleHdr(oldmem[index])) {
    u32 obj_index = *free;
    u32 len = Max(1, RawVal(oldmem[index]));
    *free += len + 1;
    Copy(oldmem+index, newmem+obj_index, (len+1)*sizeof(u32));
    value = ObjVal(obj_index);
  } else {
    u32 obj_index = *free;
    *free += 2;
    newmem[obj_index] = oldmem[index];
    newmem[obj_index+1] = oldmem[index+1];
    value = ObjVal(obj_index);
  }

  oldmem[index] = moved;
  oldmem[index+1] = value;
  return value;
}

void CollectGarbage(void)
{
  u32 i, scan;
  u32 *oldmem = mem.data;

  if (!mem.data) {
    InitMem(MIN_CAPACITY);
    return;
  }

  /* fprintf(stderr, "GARBAGE DAY!!!\n"); */

  mem.data = malloc(mem.capacity*sizeof(u32));
  mem.data[0] = 0;
  mem.data[1] = 0;
  mem.free = 2;

  for (i = 0; i < mem.num_roots; i++) {
    mem.roots[i] = CopyObj(mem.roots[i], oldmem, mem.data, &mem.free);
  }

  for (i = mem.stack; i < mem.capacity; i++) {
    mem.data[i] = CopyObj(oldmem[i], oldmem, mem.data, &mem.free);
  }

  mem.tmp[0] = CopyObj(mem.tmp[0], oldmem, mem.data, &mem.free);
  mem.tmp[1] = CopyObj(mem.tmp[1], oldmem, mem.data, &mem.free);

  scan = 2;
  while (scan < mem.free) {
    u32 next = mem.data[scan];
    if (IsBinHdr(next)) {
      scan += Max(2, BinSpace(RawVal(next)) + 1);
    } else if (IsTupleHdr(next)) {
      for (i = 0; i < RawVal(next); i++) {
        mem.data[scan+i+1] = CopyObj(mem.data[scan+i+1], oldmem, mem.data, &mem.free);
      }
      scan += Max(2, RawVal(next) + 1);
    } else {
      mem.data[scan] = CopyObj(mem.data[scan], oldmem, mem.data, &mem.free);
      mem.data[scan+1] = CopyObj(mem.data[scan+1], oldmem, mem.data, &mem.free);
      scan += 2;
    }
  }

  free(oldmem);

  if (MemFree() < MemCapacity()/4) {
    SizeMem(2*MemCapacity());
  } else if (MemCapacity() > MIN_CAPACITY*2 &&
      MemFree() > MemCapacity()/4 + MemCapacity()/2) {
    SizeMem(MemCapacity()/2);
  }
}

u32 StackPush(u32 value)
{
  if (mem.stack <= mem.free) {
    Save(value, 0);
    CollectGarbage();
    Restore(value, 0);
  }
  assert(mem.stack > mem.free);
  mem.data[--mem.stack] = value;
  return value;
}

u32 StackPop(void)
{
  assert(mem.stack < mem.capacity);
  return mem.data[mem.stack++];
}

u32 StackPeek(u32 index)
{
  assert(StackSize() > index);
  return mem.data[mem.stack + index];
}

u32 StackSize(void)
{
  return mem.capacity - mem.stack;
}

u32 Pair(u32 head, u32 tail)
{
  i32 index = MemAlloc(2);
  MemSet(index, head);
  MemSet(index+1, tail);
  return ObjVal(index);
}

u32 Head(u32 pair)
{
  return MemGet(RawVal(pair));
}

u32 Tail(u32 pair)
{
  return MemGet(RawVal(pair)+1);
}

u32 ObjLength(u32 obj)
{
  return RawVal(mem.data[RawVal(obj)]);
}

u32 Tuple(u32 length)
{
  u32 i;
  u32 index = MemAlloc(length+1);
  MemSet(index, TupleHeader(length));
  for (i = 0; i < length; i++) MemSet(index + i + 1, 0);
  return ObjVal(index);
}

u32 TupleGet(u32 tuple, u32 index)
{
  if (index < 0 || index >= ObjLength(tuple)) return 0;
  return MemGet(RawVal(tuple)+index+1);
}

void TupleSet(u32 tuple, u32 index, u32 value)
{
  if (index < 0 || index >= ObjLength(tuple)) return;
  MemSet(RawVal(tuple)+index+1, value);
}

u32 TupleJoin(u32 left, u32 right)
{
  u32 i;
  u32 tuple;
  Save(left, 0);
  Save(right, 1);
  tuple = Tuple(ObjLength(left) + ObjLength(right));
  Restore(left, 0);
  Restore(right, 1);

  for (i = 0; i < ObjLength(left); i++) {
    TupleSet(tuple, i, TupleGet(left, i));
  }
  for (i = 0; i < ObjLength(right); i++) {
    TupleSet(tuple, i+ObjLength(left), TupleGet(right, i));
  }
  return tuple;
}

u32 TupleSlice(u32 tuple, u32 start, u32 end)
{
  u32 i;
  u32 len = (end > start) ? end - start : 0;
  u32 slice;

  Save(tuple, 0);
  slice = Tuple(Min(len, ObjLength(tuple)));
  Restore(tuple, 0);

  for (i = 0; i < ObjLength(slice); i++) {
    TupleSet(slice, i, TupleGet(tuple, i + start));
  }
  return slice;
}

u32 NewBinary(u32 length)
{
  u32 index = MemAlloc(BinSpace(length) + 1);
  MemSet(index, BinHeader(length));
  return ObjVal(index);
}

u32 Binary(char *str)
{
  return BinaryFrom(str, StrLen(str));
}

u32 BinaryFrom(char *str, u32 length)
{
  u32 bin = NewBinary(length);
  char *binData = BinaryData(bin);
  Copy(str, binData, length);
  return bin;
}

char *BinaryData(u32 bin)
{
  return (char*)(mem.data + RawVal(bin) + 1);
}

u32 BinaryGet(u32 bin, u32 index)
{
  if (index < 0 || index >= ObjLength(bin)) return 0;
  return BinaryData(bin)[index];
}

void BinarySet(u32 bin, u32 index, u32 value)
{
  if (index < 0 || index >= ObjLength(bin)) return;
  BinaryData(bin)[index] = value;
}

u32 BinaryJoin(u32 left, u32 right)
{
  u32 bin;
  Save(left, 0);
  Save(right, 1);
  bin = NewBinary(ObjLength(left) + ObjLength(right));
  Restore(left, 0);
  Restore(right, 1);

  Copy(BinaryData(left), BinaryData(bin), ObjLength(left));
  Copy(BinaryData(right), BinaryData(bin) + ObjLength(left), ObjLength(right));
  return bin;
}

u32 BinarySlice(u32 bin, u32 start, u32 end)
{
  u32 len = (end > start) ? end - start : 0;
  u32 slice;
  Save(bin, 0);
  slice = NewBinary(Min(len, ObjLength(bin)));
  Restore(bin, 0);

  Copy(BinaryData(bin)+start, BinaryData(slice), ObjLength(slice));
  return slice;
}

bool BinIsPrintable(u32 bin)
{
  u32 i;
  for (i = 0; i < ObjLength(bin); i++) {
    if (!IsPrintable(BinaryGet(bin, i))) return false;
  }
  return true;
}

char *BinToStr(u32 bin)
{
  char *str = malloc(ObjLength(bin) + 1);
  Copy(BinaryData(bin), str, ObjLength(bin));
  str[ObjLength(bin)] = 0;
  return str;
}


static u32 FormatSize(u32 value)
{
  if (IsInt(value) && RawInt(value) >= 0 && RawInt(value) < 256) return 1;
  if (IsBinary(value)) return ObjLength(value);
  if (IsTuple(value)) {
    u32 i, len = 0;
    for (i = 0; i < ObjLength(value); i++) {
      len += FormatSize(TupleGet(value, i));
    }
    return len;
  }
  if (value && IsPair(value)) {
    return FormatSize(Head(value)) + FormatSize(Tail(value));
  }
  return 0;
}

static u8 *FormatValInto(u32 value, u8 *buf)
{
  if (IsInt(value) && RawInt(value) >= 0 && RawInt(value) < 256) {
    *buf = (u8)RawInt(value);
    return buf + 1;
  }
  if (IsBinary(value)) {
    Copy(BinaryData(value), buf, ObjLength(value));
    return buf + ObjLength(value);
  }
  if (IsTuple(value)) {
    u32 i;
    for (i = 0; i < ObjLength(value); i++) {
      buf = FormatValInto(TupleGet(value, i), buf);
    }
    return buf;
  }
  if (value && IsPair(value)) {
    buf = FormatValInto(Head(value), buf);
    buf = FormatValInto(Tail(value), buf);
    return buf;
  }
  return buf;
}

u32 FormatVal(u32 value)
{
  u32 size, bin;
  if (IsBinary(value)) return value;
  size = FormatSize(value);
  Save(value, 0);
  bin = NewBinary(size);
  Restore(value, 0);
  FormatValInto(value, (u8*)BinaryData(bin));
  return bin;
}

bool ValEq(u32 a, u32 b)
{
  if (a == b) {
    return true;
  } else if (IsPair(a) && IsPair(b)) {
    return ValEq(Head(a), Head(b)) && ValEq(Tail(a), Tail(b));
  } else if (IsTuple(a) && IsTuple(b)) {
    u32 i;
    if (ObjLength(a) != ObjLength(b)) return false;
    for (i = 0; i < ObjLength(a); i++) {
      if (!ValEq(TupleGet(a, i), TupleGet(b, i))) return false;
    }
    return true;
  } else if (IsBinary(a) && IsBinary(b)) {
    char *adata = BinaryData(a);
    char *bdata = BinaryData(b);
    u32 i;
    if (ObjLength(a) != ObjLength(b)) return false;
    for (i = 0; i < ObjLength(a); i++) {
      if (adata[i] != bdata[i]) return false;
    }
    return true;
  } else {
    return false;
  }
}

static u32 HashValRec(u32 value, HashMap *map)
{
  u32 hash = EmptyHash;
  u32 h = Hash(&value, sizeof(value));
  if (HashMapContains(map, h)) return HashMapGet(map, h);

  HashMapSet(map, h, IntVal(hash));

  if (IsSymbol(value)) {
    hash = value;
  } else if (IsInt(value) || value == 0) {
    hash = IntVal(Hash(&value, sizeof(value)));
  } else if (IsPair(value)) {
    hash = IntVal(HashValRec(Head(value), map) ^ HashValRec(Tail(value), map));
  } else if (IsTuple(value)) {
    u32 i;
    for (i = 0; i < ObjLength(value); i++) {
      hash ^= HashValRec(TupleGet(value, i), map);
    }
    hash = IntVal(hash);
  } else if (IsBinary(value)) {
    hash = IntVal(Hash(BinaryData(value), ObjLength(value)));
  }
  HashMapSet(map, h, hash);
  return hash;
}

u32 HashVal(u32 value)
{
  u32 hash;
  HashMap map = EmptyHashMap;
  hash = HashValRec(value, &map);
  DestroyHashMap(&map);
  return hash;
}

char *MemValStr(u32 value)
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
    char *data = SymbolName(RawVal(value));
    if (data) {
      str = malloc(StrLen(data) + 2);
      str[0] = ':';
      WriteStr(data, StrLen(data), str+1);
      return str;
    } else {
      str = malloc(NumDigits(value, 10) + 1);
      WriteNum(RawInt(value), str);
      return str;
    }
  }
  if (IsBinary(value) && ObjLength(value) < 8 && BinIsPrintable(value)) {
    u32 len = ObjLength(value);
    char *data = BinaryData(value);
    str = malloc(len + 3);
    str[0] = '"';
    WriteStr(data, len, str+1);
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

#ifdef DEBUG
void DumpMem(void)
{
  u32 i;
  u32 numCols = 8;
  u32 colWidth = 10;
  u32 numWidth = NumDigits(mem.free, 10);
  u32 bin_cells = 0;
  char *bin_data = 0;

  fprintf(stderr, "%*d│", numWidth, 0);
  for (i = 0; i < mem.free; i++) {
    if (bin_cells > 0) {
      fprintf(stderr, "    \"%*.*s\"│", 4, 4, bin_data);
      bin_data += 4;
      bin_cells--;
    } else {
      u32 value = mem.data[i];
      char *str = MemValStr(value);
      fprintf(stderr, "%*s│", colWidth, str);
      free(str);
      if (IsBinHdr(value)) {
        bin_cells = BinSpace(RawVal(value));
        bin_data = (char*)(mem.data + i + 1);
      }
    }

    if (i % numCols == numCols - 1) {
      fprintf(stderr, "\n");
      fprintf(stderr, "%*d│", numWidth, i+1);
    }
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "%d / %d\n", mem.free, mem.capacity);
}
#endif
