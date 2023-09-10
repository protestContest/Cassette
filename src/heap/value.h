#pragma once

typedef union {
  i32 as_i;
  float as_f;
} Val;

#define nanMask           0x7FC00000
#define typeMask          0xFFF00000

#define intMask           0x7FC00000
#define symMask           0x7FD00000
#define pairMask          0x7FE00000
#define objMask           0x7FF00000
#define tupleMask         0xFFC00000
#define binaryMask        0xFFD00000
#define mapMask           0xFFE00000

#define MakeVal(n, mask)  ((Val){.as_i = ((n) & ~typeMask) | (mask)})
#define FloatVal(n)       ((Val){.as_f = (float)(n)})
#define IntVal(n)         MakeVal((i32)(n), intMask)
#define SymVal(n)         MakeVal(n, symMask)
#define PairVal(n)        MakeVal(n, pairMask)
#define ObjVal(n)         MakeVal(n, objMask)
#define TupleHeader(n)    MakeVal(n, tupleMask)
#define BinaryHeader(n)   MakeVal(n, binaryMask)
#define MapHeader(n)      MakeVal(n, mapMask)

#define IsType(v, mask)   (((v).as_i & typeMask) == (mask))
#define IsFloat(v)        (((v).as_i & nanMask) != nanMask)
#define IsInt(v)          IsType(v, intMask)
#define IsSym(v)          IsType(v, symMask)
#define IsPair(v)         IsType(v, pairMask)
#define IsObj(v)          IsType(v, objMask)
#define IsTupleHeader(v)  IsType(v, tupleMask)
#define IsBinaryHeader(v) IsType(v, binaryMask)
#define IsMapHeader(v)    IsType(v, mapMask)

#define SignExt(n)        ((((n) + 0x00080000) & 0x000FFFFF) - 0x00080000)
#define valBits           20

#define RawFloat(v)       (v).as_f
#define RawInt(v)         SignExt((v).as_i)
#define RawVal(v)         ((v).as_i & ~typeMask)

#define SymbolFrom(s, l)  SymVal(HashBits(s, l, valBits))
#define SymbolFor(str)    SymbolFrom(str, StrLen(str))

#define Eq(v1, v2)        ((v1).as_i == (v2).as_i)
#define nil               PairVal(0)
#define IsNil(v)          Eq(nil, v)
#define BoolVal(v)        ((v) ? True : False)
#define IsTrue(v)         !(IsNil(v) || Eq(v, False))
#define IsNum(v)          (IsFloat(v) || IsInt(v))
#define RawNum(v)         (IsFloat(v) ? RawFloat(v) : RawInt(v))
#define IsZero(v)         ((IsFloat(v) && RawNum(v) == 0.0) || (IsInt(v) && RawInt(v) == 0))

#define True              (Val){.as_i = 0x7FD69399}
#define False             (Val){.as_i = 0x7FD8716C}
#define Undefined         (Val){.as_i = 0x7FD0ED47}
#define Primitive         (Val){.as_i = 0x7FDE8B53}
#define Function          (Val){.as_i = 0x7FDBE559}
#define Moved             (Val){.as_i = 0x7FDEC294}
#define Ok                (Val){.as_i = 0x7FD9C8D3}
#define Error             (Val){.as_i = 0x7FD161DF}
