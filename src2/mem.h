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
#define NumVal(n)         ((Val){.as_f = (float)(n)})
#define IntVal(n)         MakeVal(n, intMask)
#define SymVal(n)         MakeVal(n, symMask)
#define PairVal(n)        MakeVal(n, pairMask)

#define IsType(v, mask)   (((v).as_i & typeMask) == (mask))
#define IsNum(v)          (((v).as_i & nanMask) != nanMask)
#define IsInt(v)          IsType(v, intMask)
#define IsSym(v)          IsType(v, symMask)
#define IsPair(v)         IsType(v, pairMask)

#define SignExt(n)        ((((n) + 0x00080000) & 0x000FFFFF) - 0x00080000)
#define valBits           20

#define RawNum(v)         (IsNum(v) ? (v).as_f : RawInt(v))
#define RawInt(v)         SignExt((v).as_i)
#define RawVal(v)         ((v).as_i & ~typeMask)

#define SymbolFrom(s, l)  SymVal(HashBits(s, l, valBits))
#define SymbolFor(str)    SymbolFrom(str, StrLen(str))

#define Eq(v1, v2)        ((v1).as_i == (v2).as_i)
#define nil               PairVal(0)
#define IsNil(v)          Eq(nil, v)
#define BoolVal(v)        ((v) ? SymbolFor("true") : SymbolFor("false"))
#define IsTrue(v)         !(IsNil(v) || Eq(v, SymbolFor("false")))

typedef struct {
  Val *values;
  char *strings;
  Map string_map;
} Mem;

void InitMem(Mem *mem);
void DestoryMem(Mem *mem);

Val MakeSymbol(char *name, u32 length, Mem *mem);
char *SymbolName(Val symbol, Mem *mem);

Val Pair(Val head, Val tail, Mem *mem);
Val Head(Val pair, Mem *mem);
Val Tail(Val pair, Mem *mem);

void PrintVal(Val value, Mem *mem);
