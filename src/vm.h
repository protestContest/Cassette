#pragma once

#define NUM_SYMBOLS   256
#define MEMORY_SIZE   4096
#define STACK_BOTTOM  (MEMORY_SIZE - 1)

typedef union {
  float as_f;
  u32 as_v;
} Val;

typedef enum {
  NUMBER,
  INTEGER,
  SYMBOL,
  PAIR,
  TUPLE,
  BINARY
} ValType;

typedef struct {
  Val key;
  Val name;
} Symbol;

typedef struct {
  u32 exp;
  u32 env;
  u32 proc;
  u32 argl;
  Val val;
  u32 cont;
  u32 unev;
  u32 sp;
  u32 base;
  u32 free;
  Symbol symbols[NUM_SYMBOLS];
  Val mem[MEMORY_SIZE];
} VM;

extern Val nil_val;
extern Val quote_val;
extern Val ok_val;
extern Val false_val;
extern Val fn_val;
extern Val do_val;

#define nanMask     0x7FC00000

#define type1Mask   0xFFE00000
#define symMask     0x7FC00000
#define intMask     0x7FE00000

#define type2Mask   0xFFF00000
#define pairMask    0xFFC00000
#define tupleMask   0xFFE00000
#define binMask     0xFFF00000
#define xMask       0xFFD00000

#define IsType1(v)  (((v).as_v & 0x80000000) == 0x0)
#define NumVal(n)   (Val)(n)
#define IsNum(n)    (((n).as_v & nanMask) != nanMask)
#define IntVal(i)   (Val)(((i) & ~type1Mask) | intMask)
#define IsInt(i)    (((i).as_v & type1Mask) == intMask)
#define SymVal(s)   (Val)(((s) & ~type1Mask) | symMask)
#define IsSym(s)    (((s).as_v & type1Mask) == symMask)
#define PairVal(p)  (Val)(((p) & ~type2Mask) | pairMask)
#define IsPair(p)   (((p).as_v & type2Mask) == pairMask)
#define TupleVal(t) (Val)(((t) & ~type2Mask) | tupleMask)
#define IsTuple(t)  (((t).as_v & type2Mask) == tupleMask)
#define BinVal(b)   (Val)(((b) & ~type2Mask) | binMask)
#define IsBin(b)    (((b).as_v & type2Mask) == binMask)
#define IsNil(v)    ((v).as_v == nil_val.as_v)

#define RawVal(v)   (IsNum(v) ? (v).as_f : IsType1(v) ? ((v).as_v & (~type1Mask)) : ((v).as_v & (~type2Mask)))
#define AsInt(v)    ((v).as_v & (~type1Mask))

#define Eq(v1, v2)  ((v1).as_v == (v2).as_v)

#define IsTaggedList(vm, exp, tag)  (IsPair(exp) && Eq(Head(vm, exp), tag))

ValType TypeOf(Val v);

void InitVM(VM *vm);

#define IsStackEmpty(vm)  ((vm)->sp == STACK_BOTTOM)
void StackPush(VM *vm, Val val);
Val StackPop(VM *vm);
Val StackPeek(VM *vm, u32 i);

Val MakeSymbol(VM *vm, char *src, u32 len);
Val SymbolName(VM *vm, Val sym);
Val SymbolFor(VM *vm, char *src, u32 len);
Val BinToSymbol(VM *vm, Val bin);
char *SymbolText(VM *vm, Val sym);

Val MakePair(VM *vm, Val head, Val tail);
Val Head(VM *vm, Val pair);
Val Tail(VM *vm, Val pair);
void SetHead(VM *vm, Val pair, Val head);
void SetTail(VM *vm, Val pair, Val tail);
Val Reverse(VM *vm, Val list);

Val MakeTuple(VM *vm, u32 length, ...);
u32 TupleLength(VM *vm, Val tuple);
Val TupleAt(VM *vm, Val tuple, u32 i);

Val StrToBinary(VM *vm, char *src, u32 len);
Val MakeBinary(VM *vm, u32 len);
u32 BinaryLength(VM *vm, Val binary);
byte *BinaryData(VM *vm, Val binary);

void DumpVM(VM *vm);

