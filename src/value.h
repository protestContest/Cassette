#pragma once

typedef union {
  float as_f;
  i32 as_v;
} Val;

typedef struct {
  Status status;
  Val value;
} Result;

#define nanMask       0x7FC00000

#define typeMask      0xFFF00000
#define intMask       0xFFE00000

#define posIntMask    0x7FE00000
#define negIntMask    0x7FF00000

#define symMask       0x7FC00000
#define pairMask      0xFFC00000
#define binMask       0xFFD00000
#define tupleMask     0xFFE00000
#define mapMask       0xFFF00000

#define valBits       20

#define NumVal(n)     (Val)(float)(n)
#define IsNum(n)      (((n).as_v & nanMask) != nanMask)

#define IntVal(i)     (Val)(i32)(((i) & ~typeMask) | ((i < 0) ? negIntMask : posIntMask))
#define IsInt(i)      (((i).as_v & intMask) == intMask)
#define RawInt(v)     -(((v).as_v & ~intMask)^intMask + 1)

#define IsNumeric(n)  (IsNum(n) || IsInt(n))

#define SymVal(s)     (Val)(i32)(((s) & ~typeMask) | symMask)
#define IsSym(s)      (((s).as_v & typeMask) == symMask)

#define PairVal(p)    (Val)(i32)(((p) & ~typeMask) | pairMask)
#define IsPair(p)     (((p).as_v & typeMask) == pairMask)
#define TupleVal(t)   (Val)(i32)(((t) & ~typeMask) | tupleMask)
#define IsTuple(t)    (((t).as_v & typeMask) == tupleMask)
#define BinVal(b)     (Val)(i32)(((b) & ~typeMask) | binMask)
#define IsBin(b)      (((b).as_v & typeMask) == binMask)
#define MapVal(d)     (Val)(i32)(((d) & ~typeMask) | mapMask)
#define IsMap(d)      (((d).as_v & typeMask) == mapMask)

#define RawNum(v)     (IsNum(v) ? (v).as_f : RawInt(v))
#define RawVal(v)     ((v).as_v & ~typeMask)

#define hdrMask       0xF0000000
#define binHdrMask    0xD0000000
#define tupHdrMask    0xE0000000
#define mapHdrMask    0xF0000000

#define BinHdr(n)     (Val)(i32)(((n) & ~hdrMask) | binHdrMask)
#define IsBinHdr(h)   (((h) & hdrMask) == binHdrMask)
#define TupHdr(n)     (Val)(i32)(((n) & ~hdrMask) | tupHdrMask)
#define IsTupHdr(h)   (((h) & hdrMask) == tupHdrMask)
#define MapHdr(n)     (Val)(i32)(((n) & ~hdrMask) | mapHdrMask)
#define IsMapHdr(h)   (((h) & hdrMask) == mapHdrMask)
#define HdrVal(h)     ((h).as_v & ~hdrMask)

#define Eq(v1, v2)    ((v1).as_v == (v2).as_v)

#define nil           ((Val)(i32)0xFFC00000)
#define IsNil(v)      (Eq(v, nil))
