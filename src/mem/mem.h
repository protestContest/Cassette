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
#define bignumMask        0xFFE00000

#define MakeVal(n, mask)  (((n) & ~typeMask) | (mask))
#define IntVal(n)         MakeVal((i32)(n), intMask)
#define SymVal(n)         MakeVal(n, symMask)
#define PairVal(n)        MakeVal(n, pairMask)
#define ObjVal(n)         MakeVal(n, objMask)
#define TupleHeader(n)    MakeVal(n, tupleMask)
#define BinaryHeader(n)   MakeVal(n, binaryMask)
#define BignumHeader(n)   MakeVal(n, bignumMask)

#define IsType(v, mask)   (((v) & typeMask) == (mask))
#define IsFloat(v)        (((v) & nanMask) != nanMask)
#define IsInt(v)          IsType(v, intMask)
#define IsSym(v)          IsType(v, symMask)
#define IsPair(v)         IsType(v, pairMask)
#define IsObj(v)          IsType(v, objMask)
#define IsTupleHeader(v)  IsType(v, tupleMask)
#define IsBinaryHeader(v) IsType(v, binaryMask)
#define IsBignumHeader(v) IsType(v, bignumMask)

#define SignExt(n)        ((((n) + 0x00080000) & 0x000FFFFF) - 0x00080000)
#define valBits           20

#define RawInt(v)         ((i32)SignExt(v))
#define RawVal(v)         ((v) & ~typeMask)

#define BoolVal(v)        ((v) ? True : False)
#define IsNum(v,m)        (IsFloat(v) || IsInt(v) || IsBignum(v, m))
#define RawNum(v)         (IsFloat(v) ? RawFloat(v) : RawInt(v))
#define IsZero(v)         ((IsFloat(v) && RawNum(v) == 0.0) || (IsInt(v) && RawInt(v) == 0))
#define MulOverflows(a, b)  ((b) != 0 && (a) > RawInt(MaxIntVal) / (b))
#define MulUnderflows(a, b) ((b) != 0 && (a) < RawInt(MinIntVal) / (b))
#define AddOverflows(a, b)  ((b) > 0 && (a) > RawInt(MaxIntVal) - (b))
#define AddUnderflows(a, b) ((b) < 0 && (a) < RawInt(MinIntVal) - (b))

#define Nil               0x7FE00000
#define MaxIntVal         0x7FC7FFFF
#define MinIntVal         0x7FC80000

/* pre-computed symbols */
#define True              0x7FD7E33F /* true */
#define False             0x7FDAF4C4 /* false */
#define Ok                0x7FDDDE52 /* ok */
#define Error             0x7FD04FC6 /* error */
#define Primitive         0x7FD15938 /* *prim* */
#define Function          0x7FD4D0E4 /* *func* */
#define Undefined         0x7FDBC789 /* *undefined* */
#define Moved             0x7FDE7580 /* *moved* */
#define File              0x7FDA0003 /* *file* */
#define FloatType         0x7FDF2D07 /* float */
#define IntType           0x7FD90789 /* integer */
#define NumType           0x7FDFEE5C /* number */
#define SymType           0x7FD66184 /* symbol */
#define PairType          0x7FD2281F /* pair */
#define TupleType         0x7FDB66C6 /* tuple */
#define BinaryType        0x7FD56D87 /* binary */
#define FuncType          0x7FDC45D6 /* function */

Val FloatVal(float num);
float RawFloat(Val value);

typedef struct {
  u32 capacity;
  u32 count;
  Val *values;
} Mem;

#define NumBinCells(size)   ((size - 1) / 4 + 1)

void InitMem(Mem *mem, u32 capacity);
void DestroyMem(Mem *mem);
bool CheckCapacity(Mem *mem, u32 amount);
void ResizeMem(Mem *mem, u32 capacity);

void PushMem(Mem *mem, Val value);
Val PopMem(Mem *mem);

Val TypeOf(Val value, Mem *mem);

Val Pair(Val head, Val tail, Mem *mem);
Val Head(Val pair, Mem *mem);
Val Tail(Val pair, Mem *mem);
void SetHead(Val pair, Val value, Mem *mem);
void SetTail(Val pair, Val value, Mem *mem);
u32 ListLength(Val list, Mem *mem);
bool ListContains(Val list, Val item, Mem *mem);
Val ReverseList(Val list, Val tail, Mem *mem);
Val ListGet(Val list, u32 index, Mem *mem);
Val ListCat(Val list1, Val list2, Mem *mem);

Val MakeTuple(u32 length, Mem *mem);
bool IsTuple(Val value, Mem *mem);
u32 TupleLength(Val tuple, Mem *mem);
bool TupleContains(Val tuple, Val item, Mem *mem);
void TupleSet(Val tuple, u32 index, Val value, Mem *mem);
Val TupleGet(Val tuple, u32 index, Mem *mem);
Val TupleCat(Val tuple1, Val tuple2, Mem *mem);

Val MakeBinary(u32 size, Mem *mem);
Val BinaryFrom(char *str, u32 size, Mem *mem);
bool IsBinary(Val value, Mem *mem);
u32 BinaryLength(Val binary, Mem *mem);
bool BinaryContains(Val binary, Val item, Mem *mem);
void *BinaryData(Val binary, Mem *mem);
Val BinaryCat(Val binary1, Val binary2, Mem *mem);

Val MakeBignum(i64 num, Mem *mem);
bool IsBignum(Val value, Mem *mem);
bool BignumGreater(Val a, Val b, Mem *mem);
bool BignumEq(Val a, Val b, Mem *mem);
Val BignumAdd(Val a, Val b, Mem *mem);
Val BignumSub(Val a, Val b, Mem *mem);
Val BignumMul(Val a, Val b, Mem *mem);
Val BignumDiv(Val a, Val b, Mem *mem);
Val BignumRem(Val a, Val b, Mem *mem);

float NumToFloat(Val num, Mem *mem);
float NumToBignum(Val num, Mem *mem);
Val AddVal(Val a, Val b, Mem *mem);
Val SubVal(Val a, Val b, Mem *mem);
Val MultiplyVal(Val a, Val b, Mem *mem);
Val DivideVal(Val a, Val b, Mem *mem);
Val RemainderVal(Val a, Val b, Mem *mem);
bool ValLessThan(Val a, Val b, Mem *mem);
bool ValGreaterThan(Val a, Val b, Mem *mem);
bool NumValEqual(Val a, Val b, Mem *mem);

void CollectGarbage(Val *roots, u32 num_roots, Mem *mem);
