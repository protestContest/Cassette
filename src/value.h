#pragma once

typedef union {
  float as_f;
  i32 as_v;
} Val;

typedef struct {
  Status status;
  Val value;
} Result;

#define SignExt(n)    ((((n) + 0x00080000) & 0x000FFFFF) - 0x00080000)
#define valBits       20

#define nanMask       0x7FC00000
#define typeMask      0xFFF00000
#define intMask       0x7FC00000
#define symMask       0x7FD00000
#define pairMask      0x7FE00000
#define objMask       0x7FF00000

#define NumVal(n)     (Val){.as_f = (float)(n)}
#define IntVal(n)     (Val){.as_v = ((n) & ~typeMask) | intMask}
#define SymVal(n)     (Val){.as_v = ((n) & ~typeMask) | symMask}
#define PairVal(n)    (Val){.as_v = ((n) & ~typeMask) | pairMask}
#define ObjVal(n)     (Val){.as_v = ((n) & ~typeMask) | objMask}

#define IsNum(v)      (((v).as_v & nanMask) != nanMask)
#define IsInt(v)      (((v).as_v & typeMask) == intMask)
#define IsSym(v)      (((v).as_v & typeMask) == symMask)
#define IsPair(v)     (((v).as_v & typeMask) == pairMask)
#define IsObj(v)      (((v).as_v & typeMask) == objMask)

#define RawNum(v)     (v).as_f
#define RawInt(v)     SignExt((v).as_v)
#define RawSym(v)     ((v).as_v & ~typeMask)
#define RawPair(v)    ((v).as_v & ~typeMask)
#define RawObj(v)     ((v).as_v & ~typeMask)

#define IsNumeric(n)  (IsNum(n) || IsInt(n))

#define headerMask    0xE0000000
#define tupleMask     0x00000000
#define bigIntMask    0x20000000
#define dictMask      0x40000000
#define bigDictMask   0x60000000
#define binaryMask    0x80000000
// #define xMask      0xA0000000
// #define yMask      0xC0000000
// #define zMask      0xE0000000

#define TupleHeader(n)    (Val){((n) & ~headerMask) | tupleMask}
#define BigIntHeader(n)   (Val){((n) & ~headerMask) | bigIntMask}
#define DictHeader(n)     (Val){((n) & ~headerMask) | dictMask}
#define BigDictHeader(n)  (Val){((n) & ~headerMask) | bigDictMask}
#define ProcHeader(n)     (Val){((n) & ~headerMask) | procMask}
#define BinaryHeader(n)   (Val){((n) & ~headerMask) | binaryMask}

#define HeaderVal(h)      (u32)((h).as_v & ~headerMask)

#define Eq(v1, v2)    ((v1).as_v == (v2).as_v)

#define nil           PairVal(0)
#define IsNil(v)      (Eq(v, nil))
#define IsList(m, v)  (IsPair(v) && IsPair(Tail(m, v)))
#define BoolVal(v)    ((v) ? SymbolFor("true") : SymbolFor("false"))

#define SymbolFrom(str, len)  SymVal(HashBits(str, len, valBits))
#define SymbolFor(str)        SymbolFrom(str, StrLen(str))
