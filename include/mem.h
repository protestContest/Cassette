#pragma once

typedef u32 val;
enum {pairType, objType, intType, symType, tupleHdr, binHdr};

#define typeBits        3
#define valBits         (32 - typeBits)
#define typeMask        ((1 << typeBits)-1)
#define Val(type, x)    (((x) << typeBits) | (type & typeMask))
#define ValType(x)      ((x) & typeMask)
#define RawVal(v)       (((u32)(v)) >> typeBits)
#define PairVal(x)      Val(pairType, x)
#define ObjVal(x)       Val(objType, x)
#define IntVal(x)       Val(intType, x)
#define SymVal(x)       Val(symType, x)
#define TupleHeader(x)  Val(tupleHdr, x)
#define BinHeader(x)    Val(binHdr, x)
#define IsType(v,t)     (ValType(v) == (t))
#define IsPair(v)       IsType(v, pairType)
#define IsInt(v)        IsType(v, intType)
#define IsSym(v)        IsType(v, symType)
#define IsTupleHdr(v)   IsType(v, tupleHdr)
#define IsBinHdr(v)     IsType(v, binHdr)
#define IsTuple(v)      (IsType(v, objType) && IsTupleHdr(MemGet(RawVal(v))))
#define IsBinary(v)     (IsType(v, objType) && IsBinHdr(MemGet(RawVal(v))))
#define MaxIntVal       (MaxInt >> typeBits)
#define nil             0

void InitMem(void);
u32 MemAlloc(u32 count);
u32 MemSize(void);
u32 MemFree(void);
val MemGet(u32 index);
void MemSet(u32 index, val value);

val Pair(val head, val tail);
val Head(val pair);
val Tail(val pair);
u32 ListLength(val list);
val ListGet(val list, u32 index);
val ReverseList(val list, val tail);
val ListJoin(val left, val right);
val ListTrunc(val list, u32 index);
val ListSkip(val list, u32 index);

val Tuple(u32 length);
u32 TupleLength(val tuple);
val TupleGet(val tuple, u32 index);
void TupleSet(val tuple, u32 index, val value);
val TupleJoin(val left, val right);
val TupleTrunc(val list, u32 index);
val TupleSkip(val list, u32 index);

val Binary(u32 length);
val BinaryFrom(char *data, u32 length);
u32 BinaryLength(val bin);
char *BinaryData(val bin);
val BinaryGet(val bin, u32 index);
void BinarySet(val bin, u32 index, val value);
val BinaryJoin(val left, val right);
val BinaryTrunc(val list, u32 index);
val BinarySkip(val list, u32 index);
bool BinIsPrintable(val bin);

char *ValStr(val value);
