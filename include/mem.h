#pragma once

/* The runtime dynamic memory system.

Cassette values are 32 bits long. The low bit determines whether the value is an
immediate integer or a boxed value. For boxed values, the second bit determines
whether the value is a pointer or an object header. Object headers use two
additional bits to determine the object type. Object headers are used for
garbage collection, and should only appear in the heap.

xxx1: integer
xx00: pointer
xx10: object header
0010: tuple header (number of items)
0110: binary header (number of bytes)
1010: map header (bitfield)
1110: function header (arity)

The value of a pointer is the heap index of its contents. If the value at that
index is not an object header, the value is a pair and implicitly contains the
two values on the heap at that index. If the value is an object header, the
value is that object, and the header contains type-dependent info.

A tuple header contains the count of items, and is followed by the items. A
binary header contains the count of bytes and is followed by the data, padded to
4 bytes. A map header contains the bitfield of slots and is followed by the
slots. A function header contains the function arity and is followed by a code
index (the entrypoint) and the function's environment.

Some common operations are required for tuples and binaries:

- Length: returns the length of the object
- Get: returns an element of the object
- Join: concatenates two objects, returning a new one
- Slice: returns a subset of an object
*/

enum {tupleHdr, binHdr, mapHdr};

#define IntVal(n)       (((n) << 1) + 1)
#define RawInt(n)       ((i32)(n) >> 1)
#define RawSym(n)       ((u32)(n) >> 1)
#define PtrVal(n)       ((n) << 2)
#define RawPtr(n)       ((u32)(n) >> 2)
#define HdrVal(n,t)     (((((n) << 2) | (t)) << 2) | 2)
#define RawHdr(n)       ((n) >> 4)
#define HdrType(n)      (((n) >> 2) & 0x3)
#define TupleHeader(n)  HdrVal(n, tupleHdr)
#define BinHeader(n)    HdrVal(n, binHdr)
#define MapHeader(n)    HdrVal(n, mapHdr)

#define IsInt(n)        ((n) & 1)
#define IsSymbol(n)     (IsInt(n) && SymbolName(RawInt(n)))
#define IsPtr(n)        (((n) & 0x3) == 0)
#define IsHdr(n)        (((n) & 0x3) == 0x2)
#define IsPair(n)       (IsPtr(n) && !IsHdr(Head(n)))
#define IsObj(n)        (IsPtr(n) && IsHdr(Head(n)))
#define IsTupleHdr(n)   (((n) & 0xF) == 0x2)
#define IsBinHdr(n)     (((n) & 0xF) == 0x6)
#define IsMapHdr(n)     (((n) & 0xF) == 0xA)
#define IsFnHdr(n)      (((n) & 0xF) == 0xE)
#define IsTuple(n)      (IsPtr(n) && IsTupleHdr(Head(n)))
#define IsBinary(n)     (IsPtr(n) && IsBinHdr(Head(n)))
#define IsMap(n)        (IsPtr(n) && IsMapHdr(Head(n)))

#define ObjType(n)      HdrType(Head(n))
#define HeaderVal(n)    RawHdr(Head(n))

#define MaxIntVal       0x7FFFFFFF
#define MinIntVal       0x80000001
#define nil             0

typedef struct {
  u32 capacity;
  u32 free;
  u32 stack;
  u32 *data;
} Mem;

void InitMem(u32 size);
void DestroyMem(void);
void SizeMem(u32 size);
u32 MemCapacity(void);
u32 MemFree(void);
void CollectGarbage(u32 *roots, u32 num_roots);

void StackPush(u32 value);
u32 StackPop(void);
u32 StackPeek(u32 index);
u32 StackSize(void);

u32 Pair(u32 head, u32 tail);
u32 Head(u32 pair);
u32 Tail(u32 pair);

#define ObjLength(obj) HeaderVal(obj)

u32 Tuple(u32 length);
u32 TupleGet(u32 tuple, u32 index);
void TupleSet(u32 tuple, u32 index, u32 value);
u32 TupleJoin(u32 left, u32 right);
u32 TupleSlice(u32 tuple, u32 start, u32 end);

#define BinSpace(length)  (Align(length, sizeof(u32)) / sizeof(u32))
u32 NewBinary(u32 length);
u32 Binary(char *str);
u32 BinaryFrom(char *data, u32 length);
char *BinaryData(u32 bin);
u32 BinaryGet(u32 bin, u32 index);
void BinarySet(u32 bin, u32 index, u32 value);
u32 BinaryJoin(u32 left, u32 right);
u32 BinarySlice(u32 list, u32 start, u32 end);
bool BinIsPrintable(u32 bin);
char *BinToStr(u32 bin);

bool ValEq(u32 a, u32 b);
u32 HashVal(u32 value);
u32 InspectVal(u32 value);
char *MemValStr(u32 value);
#ifdef DEBUG
void DumpMem(void);
#endif
