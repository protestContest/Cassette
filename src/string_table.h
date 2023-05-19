#pragma once
#include "value.h"

typedef struct {
  Map symbols;
  u32 *positions;
  u8 *data;
} StringTable;

void InitStringTable(StringTable *table);
void DestroyStringTable(StringTable *table);
Val PutString(StringTable *table, char *str, u32 length);
char *GetString(StringTable *table, Val symbol);
u32 GetStringLength(StringTable *table, Val symbol);

void PrintStringTable(StringTable *table);
