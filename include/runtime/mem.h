#pragma once

/*
 * The runtime dynamic memory system.
 *
 * Cassette values are 32 bits long, using 2 bits as a type tag. The types are object, integer,
 * tuple header, and binary header. An object is a pointer to a pair, a tuple header, or a binary
 * header. Tuple and binary headers should only appear in the heap. An object value is considered to
 * "be" a pair, tuple, or binary if that's what it points to.
 *
 * A tuple header contains the count of its items, followed by the items. A binary header contains
 * its length in bytes, followed by its binary data. Since heap space is allocated in 32-bit cells,
 * a binary is padded to the next cell boundary.
 *
 * Values can be stored on the stack, and objects can be created in the heap. If there isn't enough
 * space, garbage is collected and the heap is potentially resized.
 *
 * Before calling any function that may collect garbage, all live objects (except the function
 * arguments) must be on the stack, in the heap, or in the root values. Those objects must be read
 * back after the call, since their values may have changed.
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
#define MinIntVal       0x80000001

typedef struct {
  u32 capacity;
  u32 free;
  u32 stack;
  u32 tmp[2];
  u32 *data;
  u32 *roots;
  u32 num_roots;
  bool collecting;
} Mem;

void InitMem(u32 size);
void DestroyMem(void);
void SizeMem(u32 size);
u32 MemCapacity(void);
u32 MemFree(void);
void SetMemRoots(u32 *roots, u32 num_roots);
void CollectGarbage(void);

u32 StackPush(u32 value); /* may GC */
u32 StackPop(void);
u32 StackPeek(u32 index);
u32 StackSize(void);

u32 Pair(u32 head, u32 tail); /* may GC */
u32 Head(u32 pair);
u32 Tail(u32 pair);

u32 ObjLength(u32 obj);

u32 Tuple(u32 length); /* may GC */
u32 TupleGet(u32 tuple, u32 index);
void TupleSet(u32 tuple, u32 index, u32 value);
u32 TupleJoin(u32 left, u32 right); /* may GC */
u32 TupleSlice(u32 tuple, u32 start, u32 end); /* may GC */

#define BinSpace(length)  (Align(length, sizeof(u32)) / sizeof(u32))
u32 NewBinary(u32 length); /* may GC */
u32 Binary(char *str); /* may GC */
u32 BinaryFrom(char *data, u32 length); /* may GC */
char *BinaryData(u32 bin);
u32 BinaryGet(u32 bin, u32 index);
void BinarySet(u32 bin, u32 index, u32 value);
u32 BinaryJoin(u32 left, u32 right); /* may GC */
u32 BinarySlice(u32 list, u32 start, u32 end); /* may GC */
bool BinIsPrintable(u32 bin);
char *BinToStr(u32 bin);

u32 FormatVal(u32 value); /* may GC */

bool ValEq(u32 a, u32 b);
u32 HashVal(u32 a);
u32 InspectVal(u32 value);
char *MemValStr(u32 value);

#ifdef DEBUG
#include "runtime/stats.h"
extern StatGroup *mem_stats;
void DumpMem(void);
#endif
