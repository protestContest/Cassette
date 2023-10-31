#pragma once

#include "univ.h"
typedef u32 Val;

#define nanMask           0x7FC00000
#define typeMask          0xFFF00000

#define intMask           0x7FC00000
#define symMask           0x7FD00000
#define pairMask          0x7FE00000
#define objMask           0x7FF00000
#define tupleMask         0xFFC00000
#define binaryMask        0xFFD00000

#define MakeVal(n, mask)  (((n) & ~typeMask) | (mask))
#define IntVal(n)         MakeVal((i32)(n), intMask)
#define SymVal(n)         MakeVal(n, symMask)
#define PairVal(n)        MakeVal(n, pairMask)
#define ObjVal(n)         MakeVal(n, objMask)
#define TupleHeader(n)    MakeVal(n, tupleMask)
#define BinaryHeader(n)   MakeVal(n, binaryMask)

#define IsType(v, mask)   (((v) & typeMask) == (mask))
#define IsFloat(v)        (((v) & nanMask) != nanMask)
#define IsInt(v)          IsType(v, intMask)
#define IsSym(v)          IsType(v, symMask)
#define IsPair(v)         IsType(v, pairMask)
#define IsObj(v)          IsType(v, objMask)
#define IsTupleHeader(v)  IsType(v, tupleMask)
#define IsBinaryHeader(v) IsType(v, binaryMask)

#define SignExt(n)        ((((n) + 0x00080000) & 0x000FFFFF) - 0x00080000)
#define valBits           20

#define RawInt(v)         SignExt(v)
#define RawVal(v)         ((v) & ~typeMask)

#define IsNil(v)          (Nil == (v))
#define BoolVal(v)        ((v) ? True : False)
#define IsNum(v)          (IsFloat(v) || IsInt(v))
#define RawNum(v)         (IsFloat(v) ? RawFloat(v) : RawInt(v))
#define IsZero(v)         ((IsFloat(v) && RawNum(v) == 0.0) || (IsInt(v) && RawInt(v) == 0))

#define Nil               0x7FE00000

/* pre-computed symbols */
#define True              0x7FD9395C
#define False             0x7FDE2C6F
#define Ok                0x7FD72346
#define Error             0x7FDC3AAA
#define Primitive         0x7FD6E58F
#define Function          0x7FDE36D4
#define Moved             0x7FD162D1
#define Undefined         0x7FD19F74
#define File              0x7FD934AA

Val FloatVal(float num);
float RawFloat(Val value);

typedef struct {
  u32 count;
  u32 capacity;
  Val **values;
} Mem;

#define NumBinCells(size)   ((size - 1) / 4 + 1)

void InitMem(Mem *mem, u32 capacity);
void DestroyMem(Mem *mem);
bool CheckCapacity(Mem *mem, u32 amount);
Val Pair(Val head, Val tail, Mem *mem);
Val Head(Val pair, Mem *mem);
Val Tail(Val pair, Mem *mem);
void SetHead(Val pair, Val value, Mem *mem);
void SetTail(Val pair, Val value, Mem *mem);
u32 ListLength(Val list, Mem *mem);
bool ListContains(Val list, Val item, Mem *mem);
Val ReverseList(Val list, Mem *mem);
Val MakeTuple(u32 length, Mem *mem);
bool IsTuple(Val value, Mem *mem);
u32 TupleLength(Val tuple, Mem *mem);
bool TupleContains(Val tuple, Val item, Mem *mem);
void TupleSet(Val tuple, u32 index, Val value, Mem *mem);
Val TupleGet(Val tuple, u32 index, Mem *mem);
Val MakeBinary(u32 length, Mem *mem);
Val BinaryFrom(char *str, Mem *mem);
bool IsBinary(Val value, Mem *mem);
u32 BinaryLength(Val binary, Mem *mem);
void *BinaryData(Val binary, Mem *mem);
void CollectGarbage(Val *roots, u32 num_roots, Mem *mem);

struct SymbolTable;
u32 PrintVal(Val value, struct SymbolTable *symbols);
void DumpMem(Mem *mem, struct SymbolTable *symbols);
