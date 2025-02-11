#pragma once

/*
The runtime dynamic memory system.

Cassette values are 32 bits long, using 2 bits as a type tag. The types are
object, integer, tuple header, and binary header. An Object is a pointer to
a pair, a tuple header, or a binary header. Tuple and binary headers should only
appear in the heap. An object value is considered to "be" a pair, tuple, or
binary if that's what it points to.

A tuple header contains the count of its items, followed by the items. A binary
header contains its length in bytes, followed by its binary data. Since heap
space is allocated in 32-bit cells, a binary is padded to the next cell
boundary.

Values can be stored on the stack, and objects can be created in the heap.
Before pushing to the stack or creating an object, callers must ensure there is
enough memory and either collect garbage or resize memory if not.

When garbage collecting, care must be taken that all live objects are on the
stack, in the heap, or in the root values.
*/

enum {objType, intType, tupleHdr, binHdr};

#define typeBits        2
#define valBits         (32 - typeBits)
#define typeMask        ((1 << typeBits)-1)
#define Val(type, x)    (((x) << typeBits) | (type & typeMask))
#define ValType(x)      ((x) & typeMask)
#define RawVal(v)       (((u32)(v)) >> typeBits)
#define RawInt(v)       ((i32)((RawVal(v)^(1<<(valBits-1)))-(1 << (valBits-1))))
#define ObjVal(x)       Val(objType, x)
#define IntVal(x)       Val(intType, x)
#define TupleHeader(x)  Val(tupleHdr, x)
#define BinHeader(x)    Val(binHdr, x)
#define IsType(v,t)     (ValType(v) == (t))
#define IsObj(v)        IsType(v, objType)
#define IsInt(v)        IsType(v, intType)
#define IsSymbol(v)     (IsInt(v) && SymbolName(RawVal(v)))
#define IsTupleHdr(v)   IsType(v, tupleHdr)
#define IsBinHdr(v)     IsType(v, binHdr)
#define IsPair(v)       (IsObj(v) && !IsTupleHdr(Head(v)) && !IsBinHdr(Head(v)))
#define IsTuple(v)      (IsObj(v)  && IsTupleHdr(Head(v)))
#define IsBinary(v)     (IsObj(v) && IsBinHdr(Head(v)))
#define MaxIntVal       0x7FFFFFFD
#define MinIntVal       0xFFFFFFFD

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

u32 ObjLength(u32 obj);

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
u32 HashVal(u32 a);
u32 InspectVal(u32 value);
char *MemValStr(u32 value);

#ifdef DEBUG
#include "runtime/stats.h"
extern StatGroup *mem_stats;
void DumpMem(void);
#endif
