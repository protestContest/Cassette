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
#define LambdaHeader(x) Val(lambdaHdr, x)
#define SliceHeader(x)  Val(sliceHdr, x)
#define IsType(v,t)     (ValType(v) == (t))
#define IsPair(v)       IsType(v, pairType)
#define IsInt(v)        IsType(v, intType)
#define IsSym(v)        IsType(v, symType)

#define MaxIntVal       (MaxInt >> typeBits)
#define nil             0

void InitMem(void);
i32 MemAlloc(i32 count);
i32 MemSize(void);
val MemGet(i32 index);
void MemSet(i32 index, val value);

val Pair(val head, val tail);
val Head(val pair);
val Tail(val pair);
i32 ListLength(val list);
val ListGet(val list, i32 index);
val ReverseList(val list);

val Tuple(i32 length);
val Binary(i32 length);
i32 ObjLength(val tuple);
val ObjGet(val tuple, i32 index);
void ObjSet(val tuple, i32 index, val value);
