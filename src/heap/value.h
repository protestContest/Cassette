#pragma once

typedef u32 Val;

#define nanMask           0x7FC00000
#define typeMask          0xFFF00000

#define intMask           0x7FC00000
#define symMask           0x7FD00000
#define pairMask          0x7FE00000
#define objMask           0x7FF00000
#define tupleMask         0xFFC00000
#define binaryMask        0xFFD00000
#define mapMask           0xFFE00000

#define MakeVal(n, mask)  (((n) & ~typeMask) | (mask))
#define FloatVal(n)       ((Val)((f32)(n)))
#define IntVal(n)         MakeVal((i32)(n), intMask)
#define SymVal(n)         MakeVal(n, symMask)
#define PairVal(n)        MakeVal(n, pairMask)
#define ObjVal(n)         MakeVal(n, objMask)
#define TupleHeader(n)    MakeVal(n, tupleMask)
#define BinaryHeader(n)   MakeVal(n, binaryMask)
#define MapHeader(n)      MakeVal(n, mapMask)

#define IsType(v, mask)   (((v) & typeMask) == (mask))
#define IsFloat(v)        (((v) & nanMask) != nanMask)
#define IsInt(v)          IsType(v, intMask)
#define IsSym(v)          IsType(v, symMask)
#define IsPair(v)         IsType(v, pairMask)
#define IsObj(v)          IsType(v, objMask)
#define IsTupleHeader(v)  IsType(v, tupleMask)
#define IsBinaryHeader(v) IsType(v, binaryMask)
#define IsMapHeader(v)    IsType(v, mapMask)

#define SignExt(n)        ((((n) + 0x00080000) & 0x000FFFFF) - 0x00080000)
#define valBits           20

#define RawFloat(v)       ((f32)(v))
#define RawInt(v)         SignExt(v)
#define RawVal(v)         ((v) & ~typeMask)

#define IsNil(v)          (Nil == (v))
#define BoolVal(v)        ((v) ? True : False)
#define IsTrue(v)         !(IsNil(v) || Eq(v, False))
#define IsNum(v)          (IsFloat(v) || IsInt(v))
#define RawNum(v)         (IsFloat(v) ? RawFloat(v) : RawInt(v))
#define IsZero(v)         ((IsFloat(v) && RawNum(v) == 0.0) || (IsInt(v) && RawInt(v) == 0))

#define Nil               0x7FE00000
#define True              0x7FD69399
#define False             0x7FD8716C
#define Ok                0x7FD9C8D3
#define Error             0x7FD161DF
#define Undefined         0x7FD0ED47
#define Primitive         0x7FDE8B53
#define Function          0x7FDBE559
#define Moved             0x7FDEC294
