#pragma once

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

#define nil             0

i32 MemAlloc(i32 count);
i32 MemSize(void);
i32 MemGet(i32 index);
void MemSet(i32 index, i32 value);

i32 Pair(i32 head, i32 tail);
i32 Head(i32 pair);
i32 Tail(i32 pair);
i32 ListLength(i32 list);
i32 ListGet(i32 list, i32 index);
i32 ReverseList(i32 list);

i32 Tuple(i32 length);
i32 Binary(i32 length);
i32 ObjLength(i32 tuple);
i32 ObjGet(i32 tuple, i32 index);
void ObjSet(i32 tuple, i32 index, i32 value);
