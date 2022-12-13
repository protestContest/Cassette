#pragma once

typedef enum {
  NUMBER,
  SYMBOL,
  OBJECT,
} ValType;

typedef struct {
  struct {
    ValType type;
    u32 data;
  } value;
  struct {
    u32 start;
    u32 end;
  } loc;
} Value;

extern Value nilVal;

ValType TypeOf(Value value);

Value Num(u32 n);
u32 AsNum(Value val);
Value Index(u32 pos);
u32 AsIndex(Value val);
Value Symbol(u32 hash);
u32 AsSymbol(Value val);

bool IsNil(Value value);
void PrintValue(Value value, u32 len);
char *TypeAbbr(Value value);