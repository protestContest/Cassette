#pragma once

typedef union {
  float as_f;
  i32 as_v;
} Val;

typedef enum {
  NUMBER,
  INTEGER,
  SYMBOL,
  PAIR,
  BINARY,
  TUPLE,
  DICT,
} ValType;

#define nanMask       0x7FC00000

#define type1Mask     0xFFE00000
#define symMask       0x7FC00000
#define intMask       0x7FE00000
#define negIntMask    0x7FF00000

#define type2Mask     0xFFF00000
#define pairMask      0xFFC00000
#define binMask       0xFFD00000
#define tupleMask     0xFFE00000
#define dctMask       0xFFF00000

#define IsType1(v)    (((v).as_v & 0x80000000) == 0x0)
#define NumVal(n)     (Val)(float)(n)
#define IsNum(n)      (((n).as_v & nanMask) != nanMask)
#define IntVal(i)     (Val)(i32)(((i) & ~type1Mask) | intMask)
#define IsInt(i)      (((i).as_v & type1Mask) == intMask)
#define IsNegInt(i)   (((i).as_v & type2Mask) == negIntMask)
#define SymVal(s)     (Val)(i32)(((s) & ~type1Mask) | symMask)
#define IsSym(s)      (((s).as_v & type1Mask) == symMask)
#define IsNumeric(n)  (IsNum(n) || IsInt(n))

#define PairVal(p)    (Val)(i32)(((p) & ~type2Mask) | pairMask)
#define IsPair(p)     (((p).as_v & type2Mask) == pairMask)
#define TupleVal(t)   (Val)(i32)(((t) & ~type2Mask) | tupleMask)
#define IsTuple(t)    (((t).as_v & type2Mask) == tupleMask)
#define BinVal(b)     (Val)(i32)(((b) & ~type2Mask) | binMask)
#define IsBin(b)      (((b).as_v & type2Mask) == binMask)
#define DictVal(d)    (Val)(i32)(((d) & ~type2Mask) | dctMask)
#define IsDict(d)     (((d).as_v & type2Mask) == dctMask)

#define RawInt(v)   (IsNegInt(v) ? (i32)((v).as_v | type2Mask) : (i32)((v).as_v & ~type1Mask))
#define RawNum(v)   (IsNum(v) ? (v).as_f : RawInt(v))
#define RawSym(v)   ((v).as_v & ~type1Mask)
#define RawObj(v)   ((v).as_v & ~type2Mask)

#define hdrMask     0xF0000000
#define binHdrMask  0xD0000000
#define tupHdrMask  0xE0000000
#define dctHdrMask  0xF0000000

#define BinHdr(n)   (Val)(i32)(((n) & ~hdrMask) | binHdrMask)
#define IsBinHdr(h) (((h) & hdrMask) == binHdrMask)
#define TupHdr(n)   (Val)(i32)(((n) & ~hdrMask) | tupHdrMask)
#define IsTupHdr(h) (((h) & hdrMask) == tupHdrMask)
#define DctHdr(n)   (Val)(i32)(((n) & ~hdrMask) | dctHdrMask)
#define IsDctHdr(h) (((h) & hdrMask) == dctHdrMask)
#define HdrVal(h)   ((h).as_v & ~hdrMask)

#define Eq(v1, v2)  ((v1).as_v == (v2).as_v)

#define nil           ((Val)(i32)0xFFC00000)
#define IsNil(v)      (Eq(v, nil))
#define IsList(m, v)  (IsPair(v) && !IsNil(v) && IsPair(Tail(m, v)))

#define TypeOf(v)         \
  IsNum(v)    ? NUMBER :  \
  IsInt(v)    ? INTEGER : \
  IsSym(v)    ? SYMBOL :  \
  IsPair(v)   ? PAIR :    \
  IsBin(v)    ? BINARY :  \
  IsTuple(v)  ? TUPLE :   \
  IsDict(v)   ? DICT :    \
  -1
