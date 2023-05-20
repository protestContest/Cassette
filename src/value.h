#pragma once

typedef union {
  float as_f;
  i32 as_v;
} Val;

#define nanMask           0x7FC00000
#define typeMask          0xFFF00000
#define intMask           0x7FC00000
#define symMask           0x7FD00000
#define pairMask          0x7FE00000
#define objMask           0x7FF00000
#define tupleMask         0xFFC00000
#define binaryMask        0xFFD00000

#define MakeVal(n, mask)  (Val){.as_v = ((n) & ~typeMask) | (mask)}
#define NumVal(n)         (Val){.as_f = (float)(n)}
#define IntVal(n)         MakeVal(n, intMask)
#define SymVal(n)         MakeVal(n, symMask)
#define PairVal(n)        MakeVal(n, pairMask)
#define ObjVal(n)         MakeVal(n, objMask)
#define TupleHeader(n)    MakeVal(n, tupleMask)
#define BinaryHeader(n)   MakeVal(n, binaryMask)

#define IsType(v, mask)   (((v).as_v & typeMask) == (mask))
#define IsNum(v)          (((v).as_v & nanMask) != nanMask)
#define IsInt(v)          IsType(v, intMask)
#define IsSym(v)          IsType(v, symMask)
#define IsPair(v)         IsType(v, pairMask)
#define IsObj(v)          IsType(v, objMask)
#define IsTupleHeader(v)  IsType(v, tupleMask)
#define IsBinaryHeader(v) IsType(v, binaryMask)
#define IsNumeric(n)      (IsNum(n) || IsInt(n))

#define SignExt(n)        ((((n) + 0x00080000) & 0x000FFFFF) - 0x00080000)
#define valBits           20

#define RawNum(v)         (IsNum(v) ? (v).as_f : RawInt(v))
#define RawInt(v)         SignExt((v).as_v)
#define RawVal(v)         ((v).as_v & ~typeMask)
#define ValType(v)        ((v).as_v & typeMask)

#define nil               PairVal(0)
#define IsNil(v)          (Eq(v, nil))
#define IsList(m, v)      (IsPair(v) && IsPair(Tail(m, v)))
#define BoolVal(v)        ((v) ? SymbolFor("true") : SymbolFor("false"))
#define IsTrue(v)         !(IsNil(v) || Eq(v, SymbolFor("false")))
#define Eq(v1, v2)        ((v1).as_v == (v2).as_v)

#define SymbolFrom(s, l)  SymVal(HashBits(s, l, valBits))
#define SymbolFor(str)    SymbolFrom(str, StrLen(str))
