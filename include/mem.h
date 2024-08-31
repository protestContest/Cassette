#pragma once

/* The runtime dynamic memory system.

Cassette values are 32 bits long, using 2 bits as a type tag. The types are
object, integer, tuple header, and binary header. An Object is a pointer to
a pair, a tuple header, or a binary header. These headers should only appear in
the heap.

The value of an object is the index into the heap of its referent. A tuple
header contains its length (the count of items), and is followed in memory by
the contents of the tuple. A binary header contains its length (in bytes), and
is followed in memory by its binary data. Since heap space is allocated in
32-bit cells, a binary is padded to the next cell boundary.

Some common operations are required for lists, tuples, and binaries:

- Length: returns the length of the object
- Get: returns an element of the object
- Join: concatenates two objects, returning a new one
- Slice: returns a subset of an object
*/

typedef u32 val;
enum {objType, intType, tupleHdr, binHdr};

#define typeBits        2
#define valBits         (32 - typeBits)
#define typeMask        ((1 << typeBits)-1)
#define Val(type, x)    (((x) << typeBits) | (type & typeMask))
#define ValType(x)      ((x) & typeMask)
#define RawVal(v)       (((u32)(v)) >> typeBits)
#define RawInt(v)       ((i32)((RawVal(v) ^ (1 << (valBits-1))) - (1 << (valBits-1))))
#define ObjVal(x)       Val(objType, x)
#define IntVal(x)       Val(intType, x)
#define TupleHeader(x)  Val(tupleHdr, x)
#define BinHeader(x)    Val(binHdr, x)
#define IsType(v,t)     (ValType(v) == (t))
#define IsObj(v)        IsType(v, objType)
#define IsInt(v)        IsType(v, intType)
#define IsTupleHdr(v)   IsType(v, tupleHdr)
#define IsBinHdr(v)     IsType(v, binHdr)
#define IsPair(v)       (IsObj(v) && !IsTupleHdr(Head(v)) && !IsBinHdr(Head(v)))
#define IsTuple(v)      (IsObj(v)  && IsTupleHdr(Head(v)))
#define IsBinary(v)     (IsObj(v) && IsBinHdr(Head(v)))
#define MaxIntVal       (MaxInt >> typeBits)
#define nil             0

void InitMem(u32 size);
void DestroyMem(void);
void SizeMem(u32 size);
u32 MemSize(void);
u32 MemCapacity(void);
u32 MemFree(void);
void CollectGarbage(val *roots);

val Pair(val head, val tail);
val Head(val pair);
val Tail(val pair);
u32 ListLength(val list);
val ListGet(val list, u32 index);
val ReverseList(val list, val tail);
val ListJoin(val left, val right);
val ListSlice(val list, u32 start, u32 end);

val Tuple(u32 length);
u32 TupleLength(val tuple);
val TupleGet(val tuple, u32 index);
void TupleSet(val tuple, u32 index, val value);
val TupleJoin(val left, val right);
val TupleSlice(val tuple, u32 start, u32 end);

#define BinSpace(length)  (Align(length, sizeof(val)) / sizeof(val))
val NewBinary(u32 length);
val Binary(char *str);
val BinaryFrom(char *data, u32 length);
u32 BinaryLength(val bin);
char *BinaryData(val bin);
val BinaryGet(val bin, u32 index);
void BinarySet(val bin, u32 index, val value);
val BinaryJoin(val left, val right);
val BinarySlice(val list, u32 start, u32 end);
bool BinIsPrintable(val bin);
char *BinToStr(val bin);

bool ValEq(val a, val b);
val FormatVal(val value);
val InspectVal(val value);
char *MemValStr(val value);
void DumpMem(void);
