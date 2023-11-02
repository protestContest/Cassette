#include "mem.h"
#include "symbols.h"
#include "univ.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Val FloatVal(float num)
{
  union {Val as_v; float as_f;} convert;
  convert.as_f = num;
  return convert.as_v;
}

float RawFloat(Val value)
{
  union {Val as_v; float as_f;} convert;
  convert.as_v = value;
  return convert.as_f;
}

void InitMem(Mem *mem, u32 capacity)
{
  mem->values = (Val**)NewHandle(sizeof(Val) * capacity);
  mem->count = 0;
  mem->capacity = capacity;
  Pair(Nil, Nil, mem);
}

void DestroyMem(Mem *mem)
{
  DisposeHandle((Handle)mem->values);
  mem->count = 0;
  mem->capacity = 0;
  mem->values = NULL;
}

Val Pair(Val head, Val tail, Mem *mem)
{
  Val pair = PairVal(mem->count);
  Assert(CheckCapacity(mem, 2));
  (*mem->values)[mem->count++] = head;
  (*mem->values)[mem->count++] = tail;
  return pair;
}

Val Head(Val pair, Mem *mem)
{
  Assert(IsPair(pair));
  return (*mem->values)[RawVal(pair)];
}

Val Tail(Val pair, Mem *mem)
{
  Assert(IsPair(pair));
  return (*mem->values)[RawVal(pair)+1];
}

void SetHead(Val pair, Val value, Mem *mem)
{
  (*mem->values)[RawVal(pair)] = value;
}

void SetTail(Val pair, Val value, Mem *mem)
{
  (*mem->values)[RawVal(pair)+1] = value;
}

u32 ListLength(Val list, Mem *mem)
{
  u32 length = 0;
  while (IsPair(list) && list != Nil) {
    length++;
    list = Tail(list, mem);
  }
  return length;
}

bool ListContains(Val list, Val item, Mem *mem)
{
  while (IsPair(list) && list != Nil) {
    if (Head(list, mem) == item) return true;
    list = Tail(list, mem);
  }
  return false;
}

Val ReverseList(Val list, Mem *mem)
{
  Val reversed = Nil;
  Assert(IsPair(list));
  while (list != Nil) {
    reversed = Pair(Head(list, mem), reversed, mem);
    list = Tail(list, mem);
  }
  return reversed;
}

Val MakeTuple(u32 length, Mem *mem)
{
  u32 i;
  Val tuple = ObjVal(mem->count);
  Assert(CheckCapacity(mem, length + 1));
  (*mem->values)[mem->count++] = TupleHeader(length);
  for (i = 0; i < length; i++) {
    (*mem->values)[mem->count++] = Nil;
  }
  return tuple;
}

bool IsTuple(Val value, Mem *mem)
{
  return IsObj(value) && IsTupleHeader((*mem->values)[RawVal(value)]);
}

u32 TupleLength(Val tuple, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  return RawVal((*mem->values)[RawVal(tuple)]);
}

bool TupleContains(Val tuple, Val item, Mem *mem)
{
  u32 i;
  Assert(IsTuple(tuple, mem));
  for (i = 0; i < TupleLength(tuple, mem); i++) {
    if (TupleGet(tuple, i, mem) == item) return true;
  }

  return false;
}

void TupleSet(Val tuple, u32 i, Val value, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  Assert(i < TupleLength(tuple, mem));
  (*mem->values)[RawVal(tuple) + i + 1] = value;
}

Val TupleGet(Val tuple, u32 i, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  Assert(i < TupleLength(tuple, mem));
  return (*mem->values)[RawVal(tuple) + i + 1];
}

Val MakeBinary(u32 size, Mem *mem)
{
  Val binary = ObjVal(mem->count);
  u32 cells = (size == 0) ? 1 : NumBinCells(size);
  u32 i;

  Assert(CheckCapacity(mem, cells + 1));
  (*mem->values)[mem->count++] = BinaryHeader(size);

  for (i = 0; i < cells; i++) {
    (*mem->values)[mem->count++] = 0;
  }
  return binary;
}

Val BinaryFrom(char *str, u32 size, Mem *mem)
{
  Val bin = MakeBinary(size, mem);
  char *data = BinaryData(bin, mem);
  Copy(str, data, size);
  return bin;
}

bool IsBinary(Val value, Mem *mem)
{
  return IsObj(value) && IsBinaryHeader((*mem->values)[RawVal(value)]);
}

u32 BinaryLength(Val binary, Mem *mem)
{
  Assert(IsBinary(binary, mem));
  return RawVal((*mem->values)[RawVal(binary)]);
}

void *BinaryData(Val binary, Mem *mem)
{
  Assert(IsBinary(binary, mem));
  return &(*mem->values)[RawVal(binary)+1];
}

bool CheckCapacity(Mem *mem, u32 amount)
{
  return mem->count + amount <= mem->capacity;
}

static Val CopyValue(Val value, Mem *from, Mem *to)
{
  Val new_val = Nil;

  if (!IsObj(value) && !IsPair(value)) return value;

  if ((*from->values)[RawVal(value)] == Undefined) {
    return (*from->values)[RawVal(value)+1];
  }

  if (IsPair(value)) {
    new_val = Pair(Head(value, from), Tail(value, from), to);
  } else if (IsTuple(value, from)) {
    u32 i;
    new_val = MakeTuple(TupleLength(value, from), to);
    for (i = 0; i < TupleLength(value, from); i++) {
      TupleSet(new_val, i, TupleGet(value, i, from), to);
    }
  } else if (IsBinary(value, from)) {
    new_val = MakeBinary(BinaryLength(value, from), to);
    Copy(BinaryData(value, from), BinaryData(new_val, to), BinaryLength(value, from));
  }

  SetHead(value, Undefined, from);
  SetTail(value, new_val, from);
  return new_val;
}

void CollectGarbage(Val *roots, u32 num_roots, Mem *mem)
{
  u32 i;
  Mem new_mem;
  InitMem(&new_mem, mem->capacity);

  for (i = 0; i < num_roots; i++) {
    roots[i] = CopyValue(roots[i], mem, &new_mem);
  }

  i = 2;
  while (i < new_mem.count) {
    Val value = (*new_mem.values)[i];
    if (IsBinaryHeader(value)) {
      i += NumBinCells(RawVal(value));
    } else {
      (*new_mem.values)[i] = CopyValue((*new_mem.values)[i], mem, &new_mem);
      i++;
    }
  }

  DisposeHandle((Handle)mem->values);
  *mem = new_mem;
}

u32 PrintVal(Val value, SymbolTable *symbols)
{
  if (IsNil(value)) {
    return printf("nil");
  } else if (IsInt(value)) {
    return printf("%d", RawInt(value));
  } else if (IsFloat(value)) {
    return printf("%.2f", RawFloat(value));
  } else if (IsSym(value) && symbols) {
    return printf("%s", SymbolName(value, symbols));
  } else if (IsPair(value)) {
    return printf("p%d", RawVal(value));
  } else if (IsObj(value)) {
    return printf("o%d", RawVal(value));
  } else {
    return printf("%08X", value);
  }
}

u32 PrintValLen(Val value, SymbolTable *symbols)
{
  if (value == Nil) {
    return 3;
  } else if (IsInt(value)) {
    return snprintf(0, 0, "%d", RawInt(value));
  } else if (IsFloat(value)) {
    return snprintf(0, 0, "%.2f", RawFloat(value));
  } else if (IsSym(value) && symbols) {
    return StrLen(SymbolName(value, symbols));
  } else if (IsPair(value)) {
    return snprintf(0, 0, "%d", RawVal(value)) + 1;
  } else if (IsObj(value)) {
    return snprintf(0, 0, "%d", RawVal(value)) + 1;
  } else {
    return 8;
  }
}

static void PrintCell(u32 index, Val value, SymbolTable *symbols)
{
  u32 width = 8;

  if (IsInt(value)) {
    printf("%*d", width, RawInt(value));
  } else if (IsFloat(value)) {
    printf("%*.1f", width, RawFloat(value));
  } else if (IsSym(value) && symbols) {
    printf("%*.*s", width, width, SymbolName(value, symbols));
  } else if (IsNil(value)) {
    printf("     nil");
  } else if (IsPair(value)) {
    printf("p%*d", width-1, RawVal(value));
  } else if (IsObj(value)) {
    printf("o%*d", width-1, RawVal(value));
  } else if (IsTupleHeader(value)) {
    printf("t%*d", width-1, RawVal(value));
  } else if (IsBinaryHeader(value)) {
    printf("b%*d", width-1, RawVal(value));
  } else {
    printf("%04.4X", value);
  }
}

static u32 PrintBinData(u32 index, u32 cols, Mem *mem)
{
  u32 j, size = RawVal((*mem->values)[index]), width = 8;
  u32 cells = NumBinCells(size);

  for (j = 0; j < cells; j++) {
    Val value = (*mem->values)[index+j+1];
    u32 bytes = (j == cells-1) ? (size-1) % 4 + 1 : 4;
    bool printable  = true;
    u32 k;

    for (k = 0; k < bytes; k++) {
      if (!Printable(value >> (k*8))) {
        printable = false;
        break;
      }
    }

    if ((index+j+1) % cols == 0) printf("║%04d║", index+j+1);
    else printf("│");
    if (printable) {
      for (k = 0; k < width-bytes-2; k++) printf(" ");
      printf("\"");
      for (k = 0; k < bytes; k++) printf("%c", value >> (k*8));
      printf("\"");
    } else {
      for (k = 0; k < width-(bytes*2); k++) printf(".");
      for (k = 0; k < bytes; k++) printf("%02X", value >> (k*8));
    }
    if ((index+j+2) % cols == 0) printf("║\n");
  }
  return cells;
}

void DumpMem(Mem *mem, SymbolTable *symbols)
{
  u32 i;
  u32 cols = 8;

  printf("╔════╦");
  for (i = 0; i < cols-1; i++) printf("════════╤");
  printf("════════╗\n");

  for (i = 0; i < mem->count; i++) {
    if (i % cols == 0) printf("║%04d║", i);
    else printf("│");
    PrintCell(i, (*mem->values)[i], symbols);
    if ((i+1) % cols == 0) printf("║\n");
    if (IsBinaryHeader((*mem->values)[i])) {
      i += PrintBinData(i, cols, mem);
    }
  }
  if (i % cols != 0) {
    while (i % cols != 0) {
      printf("│        ");
      i++;
    }
    printf("║\n");
  }

  printf("╚════╩");
  for (i = 0; i < cols-1; i++) printf("════════╧");
  printf("════════╝\n");
}
