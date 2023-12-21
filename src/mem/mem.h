#pragma once
#include "univ/vec.h"

/*
Values are nan-boxed 32-bit floats. The highest 3 non-nan bits determine a
value's type, and the low 20 bits store the value. Floats, ints, and symbols are
immediate; pairs and objects are pointers; and tuple, binary, and map headers
only exist in the heap. (Generally, an object pointer is refered to as a
"tuple", "binary", or "map" based on what it points to.)
*/

typedef u32 Val;

#define nanMask           0x7FC00000
#define typeMask          0xFFF00000

#define IntType           0x7FC00000
#define SymType           0x7FD00000
#define PairType          0x7FE00000
#define ObjType           0x7FF00000
#define TupleType         0xFFC00000
#define BinaryType        0xFFD00000
#define MapType           0xFFE00000
#define BignumType        0xFFF00000

#define MakeVal(n, mask)  (((n) & ~typeMask) | (mask))
#define IntVal(n)         MakeVal((i32)(n), IntType)
#define SymVal(n)         MakeVal(n, SymType)
#define PairVal(n)        MakeVal(n, PairType)
#define ObjVal(n)         MakeVal(n, ObjType)
#define TupleHeader(n)    MakeVal(n, TupleType)
#define BinaryHeader(n)   MakeVal(n, BinaryType)
#define MapHeader(n)      MakeVal(n, MapType)
#define BignumHeader(n)   MakeVal(n, BignumType)

#define TypeOf(v)         ((v) & typeMask)
#define IsType(v, mask)   (TypeOf(v) == (mask))
#define IsFloat(v)        (((v) & nanMask) != nanMask)
#define IsInt(v)          IsType(v, IntType)
#define IsSym(v)          IsType(v, SymType)
#define IsPair(v)         IsType(v, PairType)
#define IsObj(v)          IsType(v, ObjType)
#define IsTupleHeader(v)  IsType(v, TupleType)
#define IsBinaryHeader(v) IsType(v, BinaryType)
#define IsMapHeader(v)    IsType(v, MapType)
#define IsBignumHeader(v) IsType(v, BignumType)

#define SignExt(n)        ((((n) + 0x00080000) & 0x000FFFFF) - 0x00080000)
#define valBits           20

#define RawInt(v)         ((i32)SignExt(v))
#define RawVal(v)         ((v) & ~typeMask)

#define BoolVal(v)        ((v) ? True : False)
#define IsNum(v,m)        (IsFloat(v) || IsInt(v))
#define RawNum(v)         (IsFloat(v) ? RawFloat(v) : RawInt(v))
#define IsZero(v)         ((IsFloat(v) && RawNum(v) == 0.0) || (IsInt(v) && RawInt(v) == 0))
#define MulOverflows(a, b)  ((b) != 0 && (a) > RawInt(MaxIntVal) / (b))
#define MulUnderflows(a, b) ((b) != 0 && (a) < RawInt(MinIntVal) / (b))
#define AddOverflows(a, b)  ((b) > 0 && (a) > RawInt(MaxIntVal) - (b))
#define AddUnderflows(a, b) ((b) < 0 && (a) < RawInt(MinIntVal) - (b))

#define Nil               0x7FE00000
#define MaxIntVal         0x7FC7FFFF
#define MinIntVal         0x7FC80000

Val FloatVal(float num);
float RawFloat(Val value);

typedef struct {
  u32 capacity;
  u32 count;
  Val *items;
  IntVec *roots;
} Mem;

#define MemRef(mem, v)        ((mem)->items[v])
void PushMem(Mem *mem, Val value);
void PushRoot(Mem *mem, Val value);
Val PopRoot(Mem *mem, Val value);

#define NumBinCells(size)   ((size) ? (((size) - 1) / 4 + 1) : 1)

void InitMem(Mem *mem, u32 capacity, IntVec *roots);
#define DestroyMem(mem)  DestroyVec((Vec*)mem)
#define ResizeMem(mem, capacity)  ResizeVec((Vec*)mem, sizeof(Val), capacity)

#define IsTuple(val, mem)       (IsObj(val) && IsTupleHeader(MemRef(mem, RawVal(val))))
#define IsBinary(val, mem)      (IsObj(val) && IsBinaryHeader(MemRef(mem, RawVal(val))))
#define IsMap(val, mem)         (IsObj(val) && IsMapHeader(MemRef(mem, RawVal(val))))
#define IsBignum(val, mem)      (IsObj(val) && IsBignumHeader(MemRef(mem, RawVal(val))))

#define TupleLength(val, mem)   RawVal(MemRef(mem, RawVal(val)))
#define BinaryLength(val, mem)  RawVal(MemRef(mem, RawVal(val)))

#define Head(val, mem)          MemRef(mem, RawVal(val))
#define Tail(val, mem)          MemRef(mem, RawVal(val)+1)
#define TupleGet(val, i, mem)   MemRef(mem, RawVal(val) + (i) + 1)
#define BinaryData(val, mem)    ((char*)&MemRef(mem, RawVal(val)+1))

Val Pair(Val head, Val tail, Mem *mem);
u32 ListLength(Val list, Mem *mem);
bool ListContains(Val list, Val item, Mem *mem);
Val ReverseList(Val list, Val tail, Mem *mem);
Val ListGet(Val list, u32 index, Mem *mem);
Val ListCat(Val list1, Val list2, Mem *mem);

Val MakeTuple(u32 length, Mem *mem);
bool TupleContains(Val tuple, Val item, Mem *mem);
void TupleSet(Val tuple, u32 index, Val value, Mem *mem);
Val TupleCat(Val tuple1, Val tuple2, Mem *mem);

Val MakeBinary(u32 size, Mem *mem);
Val BinaryFrom(char *str, u32 size, Mem *mem);
bool BinaryContains(Val binary, Val item, Mem *mem);
Val BinaryCat(Val binary1, Val binary2, Mem *mem);

Val MakeMap(Mem *mem);
u32 MapCount(Val map, Mem *mem);
bool MapContains(Val map, Val key, Mem *mem);
Val MapSet(Val map, Val key, Val value, Mem *mem);
Val MapDelete(Val map, Val key, Mem *mem);
Val MapGet(Val map, Val key, Mem *mem);
Val MapMerge(Val map1, Val map2, Mem *mem);
Val MapKeys(Val map, Val keys, Mem *mem);
Val MapValues(Val map, Val values, Mem *mem);
bool MapIsSubset(Val v1, Val v2, Mem *mem);

bool ValEqual(Val v1, Val v2, Mem *mem);

void CollectGarbage(Mem *mem);
