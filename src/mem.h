#pragma once

typedef union {
  i32 as_i;
  float as_f;
} Val;

typedef enum {
  ValAny,
  ValNum,
  ValInt,
  ValFloat,
  ValSym,
  ValPair,
  ValTuple,
  ValBinary,
  ValMap,
} ValType;

#define nanMask           0x7FC00000
#define typeMask          0xFFF00000
#define intMask           0x7FC00000
#define symMask           0x7FD00000
#define pairMask          0x7FE00000
#define objMask           0x7FF00000
#define tupleMask         0xFFC00000
#define binaryMask        0xFFD00000
#define mapMask           0xFFE00000
#define funcMask          0xFFF00000

#define MakeVal(n, mask)  ((Val){.as_i = ((n) & ~typeMask) | (mask)})
#define NumVal(n)         ((Val){.as_f = (float)(n)})
#define IntVal(n)         MakeVal((i32)(n), intMask)
#define SymVal(n)         MakeVal(n, symMask)
#define PairVal(n)        MakeVal(n, pairMask)
#define ObjVal(n)         MakeVal(n, objMask)
#define TupleHeader(n)    MakeVal(n, tupleMask)
#define BinaryHeader(n)   MakeVal(n, binaryMask)
#define MapHeader(n)      MakeVal(n, mapMask)
#define FuncHeader(n)     MakeVal(n, funcMask)

#define IsType(v, mask)   (((v).as_i & typeMask) == (mask))
#define IsNum(v)          (((v).as_i & nanMask) != nanMask)
#define IsInt(v)          IsType(v, intMask)
#define IsSym(v)          IsType(v, symMask)
#define IsPair(v)         IsType(v, pairMask)
#define IsObj(v)          IsType(v, objMask)
#define IsTupleHeader(v)  IsType(v, tupleMask)
#define IsBinaryHeader(v) IsType(v, binaryMask)
#define IsMapHeader(v)    IsType(v, mapMask)
#define IsFuncHeader(v)   IsType(v, funcMask)

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
#define IsNumeric(v)      (IsNum(v) || IsInt(v))
#define IsZero(v)         ((IsNum(v) && RawNum(v) == 0.0) || (IsInt(v) && RawInt(v) == 0))

typedef struct {
  Val *values;
  char *strings;
  HashMap string_map;
} Mem;

void InitMem(Mem *mem);
void DestroyMem(Mem *mem);
u32 MemSize(Mem *mem);

Val MakeSymbolFrom(char *name, u32 length, Mem *mem);
Val MakeSymbol(char *name, Mem *mem);
char *SymbolName(Val symbol, Mem *mem);

Val Pair(Val head, Val tail, Mem *mem);
Val Head(Val pair, Mem *mem);
Val Tail(Val pair, Mem *mem);
void SetHead(Val pair, Val head, Mem *mem);
void SetTail(Val pair, Val tail, Mem *mem);

bool ListContains(Val list, Val value, Mem *mem);
u32 ListLength(Val list, Mem *mem);
bool IsTagged(Val list, char *tag, Mem *mem);
Val ListConcat(Val a, Val b, Mem *mem);
Val ListFrom(Val list, u32 pos, Mem *mem);
Val ListAt(Val list, u32 pos, Mem *mem);
Val ReverseList(Val list, Mem *mem);
Val ListToBinary(Val list, Mem *mem);

Val MakeBinary(u32 num_bytes, Mem *mem);
Val BinaryFrom(char *text, Mem *mem);
bool IsBinary(Val bin, Mem *mem);
u32 BinaryLength(Val bin, Mem *mem);
char *BinaryData(Val bin, Mem *mem);

Val MakeTuple(u32 length, Mem *mem);
bool IsTuple(Val tuple, Mem *mem);
u32 TupleLength(Val tuple, Mem *mem);
bool TupleContains(Val tuple, Val value, Mem *mem);
void TupleSet(Val tuple, u32 index, Val value, Mem *mem);
Val TuplePut(Val tuple, u32 index, Val value, Mem *mem);
Val TupleGet(Val tuple, u32 index, Mem *mem);
u32 TupleIndexOf(Val tuple, Val value, Mem *mem);

Val MakeMap(u32 capacity, Mem *mem);
Val MapFrom(Val keys, Val values, Mem *mem);
bool IsMap(Val map, Mem *mem);
void MapSet(Val map, Val key, Val value, Mem *mem);
Val MapPut(Val map, Val key, Val value, Mem *mem);
Val MapPutPath(Val map, Val path, Val value, Mem *mem);
Val MapGet(Val map, Val key, Mem *mem);
Val MapDelete(Val map, Val key, Mem *mem);
bool MapContains(Val map, Val key, Mem *mem);
u32 MapCount(Val map, Mem *mem);
Val MapKeys(Val map, Mem *mem);
Val MapValues(Val map, Mem *mem);

Val MakeFunction(u32 entry, u32 arity, Val env, Mem *mem);
Val MakePrimitive(u32 num, Mem *mem);
bool IsFunction(Val func, Mem *mem);
bool IsPrimitive(Val func, Mem *mem);
u32 FunctionEntry(Val func, Mem *mem);
u32 FunctionArity(Val func, Mem *mem);
Val FunctionEnv(Val func, Mem  *mem);

bool PrintVal(Val value, Mem *mem);
u32 InspectVal(Val value, Mem *mem);
void PrintMem(Mem *mem);
void PrintSymbols(Mem *mem);

void CollectGarbage(Val *roots, Mem *mem);
