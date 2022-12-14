#pragma once

typedef enum {
  NUMBER,
  INDEX,
  SYMBOL,
  PAIR,
  VECTOR,
  TRUE,
  FALSE,
  NIL
} ValType;

typedef struct {
  ValType type;
  u32 data;
} Value;

extern Value nilVal;
extern Value trueVal;
extern Value falseVal;

ValType TypeOf(Value value);
#define IsNum(value)    (TypeOf(value) == NUMBER)
#define IsIndex(value)  (TypeOf(value) == INDEX)
#define IsSym(value)    (TypeOf(value) == SYMBOL)
#define IsPair(value)    (TypeOf(value) == PAIR)
#define IsVec(value)    (TypeOf(value) == VECTOR)
#define IsTrue(value)   (TypeOf(value) == TRUE)
#define IsFalse(value)  (TypeOf(value) == FALSE)
#define IsNil(value)    (TypeOf(value) == NIL)

Value NumVal(float n);
float AsNum(Value val);
Value IndexVal(u32 pos);
u32 AsIndex(Value val);
Value SymbolVal(u32 hash);
u32 AsSymbol(Value val);
Value PairVal(u32 p);
u32 AsPair(Value v);
Value VecVal(u32 pos);
u32 AsVec(Value vec);

void PrintValue(Value value, u32 len);
char *TypeAbbr(Value value);