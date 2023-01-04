#pragma once

typedef union {
  float as_f;
  u32 as_v;
} Val;

#define nanMask     0x7FC00000

#define type1Mask   0xFFE00000
#define intMask     0x7FE00000
#define symMask     0x7FC00000

#define type2Mask   0xFFF00000
#define pairMask    0xFFC00000
#define binMask     0xFFD00000
#define tupleMask   0xFFE00000
#define dctMask     0xFFF00000

#define IsType1(v)  (((v).as_v & 0x80000000) == 0x0)
#define NumVal(n)   (Val)(n)
#define IsNum(n)    (((n).as_v & nanMask) != nanMask)
#define IntVal(i)   (Val)(((i) & ~type1Mask) | intMask)
#define IsInt(i)    (((i).as_v & type1Mask) == intMask)
#define SymVal(s)   (Val)(((s) & ~type1Mask) | symMask)
#define IsSym(s)    (((s).as_v & type1Mask) == symMask)

#define PairVal(p)  (Val)(((p) & ~type2Mask) | pairMask)
#define IsPair(p)   (((p).as_v & type2Mask) == pairMask)
#define TupleVal(t) (Val)(((t) & ~type2Mask) | tupleMask)
#define IsTuple(t)  (((t).as_v & type2Mask) == tupleMask)
#define BinVal(b)   (Val)(((b) & ~type2Mask) | binMask)
#define IsBin(b)    (((b).as_v & type2Mask) == binMask)
#define DictVal(d)  (Val)(((d) & ~type2Mask) | dctMask)
#define IsDict(d)   (((d).as_v & type2Mask) == dctMask)

#define RawVal(v)   (IsNum(v) ? (v).as_f \
                    : IsType1(v) ? ((v).as_v & (~type1Mask)) \
                    : ((v).as_v & (~type2Mask)))

#define hdrMask     0xF0000000
#define binHdrMask  0xD0000000
#define tupHdrMask  0xE0000000
#define dctHdrMask  0xF0000000

#define BinHdr(n)   (Val)(((n) & ~hdrMask) | binHdrMask)
#define IsBinHdr(h) (((h) & hdrMask) == binHdrMask)
#define TupHdr(n)   (Val)(((n) & ~hdrMask) | tupHdrMask)
#define IsTupHdr(h) (((h) & hdrMask) == tupHdrMask)
#define DctHdr(n)   (Val)(((n) & ~hdrMask) | dctHdrMask)
#define IsDctHdr(h) (((h) & hdrMask) == dctHdrMask)
#define HdrVal(h)   ((h).as_v & ~hdrMask)

#define Eq(v1, v2)  ((v1).as_v == (v2).as_v)

extern Val nil;
#define IsNil(v)    (Eq(v, nil))
