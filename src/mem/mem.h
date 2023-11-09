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
#define IsNum(v)          (IsFloat(v) || IsInt(v))
#define RawNum(v)         (IsFloat(v) ? RawFloat(v) : RawInt(v))
#define IsZero(v)         ((IsFloat(v) && RawNum(v) == 0.0) || (IsInt(v) && RawInt(v) == 0))

#define Nil               0x7FE00000
#define MaxIntVal         0x7FC7FFFF
#define MinIntVal         0x7FC80000

/* pre-computed symbols */
#define True              0x7FD7E0EC
#define False             0x7FDAF256
#define Ok                0x7FDDD1DE
#define Error             0x7FD04F60
#define Primitive         0x7FD15AFA
#define Function          0x7FD4D1AA
#define Undefined         0x7FDBC693
#define Moved             0x7FDE7A3D
#define File              0x7FDA057E
#define FloatType         0x7FDF23F5
#define IntType           0x7FD50D9D
#define SymType           0x7FD9A156
#define PairType          0x7FD2249D
#define TupleType         0x7FDB6C7E
#define BinaryType        0x7FD5609F
#define FuncType          0x7FD41D59

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

void CollectGarbage(Val *roots, u32 num_roots, Mem *mem);
